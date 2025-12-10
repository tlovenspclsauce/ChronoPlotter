#include "ChronographParsers.h"
#include "PowderTest.h"
#include "miniz.h"
#include "untar.h"

#include "xlsxdocument.h"
#include "xlsxworksheet.h"
#include "xlsxworkbook.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonParseError>
#include <QDateTime>
#include <QDebug>
#include <QLabel>

using namespace Powder;

ChronoSeries* ChronographParsers::extractLabRadarSeries(QTextStream &csv)
{
	ChronoSeries *series = new ChronoSeries();
	series->isValid = false;
	series->deleted = false;
	series->seriesNum = -1;

	while (!csv.atEnd())
	{
		QString line = csv.readLine().replace(QString(1, QChar('\0')), "");

		// LabRadar uses semicolon (;) as delimeter
		QStringList rows(line.split(";"));

		// Only parse rows with enough columns to index
		if (rows.size() < 2)
		{
			qDebug() << "Less than 2, skipping row";
			continue;
		}

		if ((rows.size() >= 17) && (rows.at(0).compare("Shot ID") != 0))
		{
			// Parsing a velocity record
			if (series->firstDate.isNull())
			{
				series->firstDate = rows.at(15);
				qDebug() << "firstDate =" << series->firstDate;
			}

			if (series->firstTime.isNull())
			{
				series->firstTime = rows.at(16);
				qDebug() << "firstTime =" << series->firstTime;
			}

			series->muzzleVelocities.append(rows.at(1).toInt());
			qDebug() << "muzzleVelocities +=" << rows.at(1).toInt();
		}
		else if (rows.at(0).compare("Series No") == 0)
		{
			series->seriesNum = rows.at(1).toInt();
			qDebug() << "seriesNum =" << series->seriesNum;
		}
		else if (rows.at(0).compare("Units velocity") == 0)
		{
			series->velocityUnits = rows.at(1);
			series->velocityUnits.replace("fps", "ft/s");
			qDebug() << "velocityUnits =" << series->velocityUnits;
		}
	}

	// Ensure we have a valid LabRadar series
	if ((series->seriesNum == -1) || series->velocityUnits.isNull() || series->firstDate.isNull() || series->firstTime.isNull())
	{
		qDebug() << "Series does not have all expected fields set, returning invalid.";
		return series;
	}

	if (series->muzzleVelocities.empty())
	{
		qDebug() << "Series has no velocities. Likely deleted series, returning invalid.";
		return series;
	}

	// We have a valid series CSV
	series->isValid = true;

	return series;
}

QList<ChronoSeries*> ChronographParsers::extractMagnetoSpeedSeries(QTextStream &csv)
{
	// MagnetoSpeed XFR app exports .CSV files in a slightly different format
	bool xfr_export = false;

	QList<ChronoSeries*> allSeries;
	ChronoSeries *curSeries = new ChronoSeries();
	curSeries->isValid = false;
	curSeries->deleted = false;
	curSeries->seriesNum = -1;

	int i = 0;
	while (!csv.atEnd())
	{
		QString line = csv.readLine();

		// MagnetoSpeed uses comma (,) as delimeter
		QStringList rows(line.split(","));

		// Trim whitespace from cells
		QMutableStringListIterator it(rows);
		while (it.hasNext())
		{
			it.next();
			it.setValue(it.value().trimmed());
		}

		qDebug() << "Line" << i << ":" << rows;

		if (rows.size() > 0)
		{
			if (rows.at(0).compare("----") == 0)
			{
				bool useSeries = true;

				// Ensure we have a valid MagnetoSpeed series
				if (((! xfr_export) && (curSeries->seriesNum == -1)) || curSeries->velocityUnits.isNull())
				{
					qDebug() << "Series does not have all expected fields set, skipping series.";
					useSeries = false;
				}

				if (curSeries->muzzleVelocities.empty())
				{
					qDebug() << "Series has no velocities. Likely deleted or empty, skipping series..";
					useSeries = false;
				}

				if (useSeries)
				{
					// We have a valid series CSV
					curSeries->isValid = true;

					qDebug() << "Adding curSeries to allSeries";

					allSeries.append(curSeries);
				}

				curSeries = new ChronoSeries();
				curSeries->isValid = false;
				curSeries->deleted = false;
				curSeries->seriesNum = -1;
			}
			else if (rows.at(0).compare("Synced on:") == 0)
			{
				// .CSV file is exported from the MagnetoSpeed XFR app
				xfr_export = true;

				QStringList dateTime = rows.at(1).split(" ");
				if (dateTime.size() == 2)
				{
					curSeries->firstDate = dateTime.at(0);
					curSeries->firstTime = dateTime.at(1);
					qDebug() << "firstDate =" << curSeries->firstDate;
					qDebug() << "firstTime =" << curSeries->firstTime;
				}
				else
				{
					qDebug() << "Failed to split datetime cell:" << rows.at(1);
				}
			}
			else if ((rows.at(0).compare("Series") == 0) && (rows.at(2) == "Shots:"))
			{
				bool ok;
				int seriesNum = rows.at(1).toInt(&ok);
				if (ok)
				{
					// MagnetoSpeed V3 files contain an integer in the 'Series' field. Use it as the series name.
					curSeries->seriesNum = seriesNum;
					curSeries->name = new QLabel(QString("Series %1").arg(seriesNum));
					qDebug() << "seriesNum =" << curSeries->seriesNum;
				}
				else
				{
					// XFR export files contain a date in the 'Series' field. Ignore it since we're expecting to be replaced by the name in the 'Notes' field.
					qDebug() << "XFR file detected, skipping Series row";
				}
			}
			else if (rows.at(0).compare("Notes") == 0)
			{
				// Use the series name if the user entered one
				if (rows.at(1).compare("") == 0)
				{
					curSeries->name = new QLabel("Unnamed");
				}
				else
				{
					curSeries->name = new QLabel(rows.at(1));
				}

				qDebug() << "Setting name to '" << curSeries->name << "' via Notes field";
			}
			else
			{
				bool ok = false;

				// If the first cell is a valid integer, it's a velocity entry
				rows.at(0).toInt(&ok);
				if (ok)
				{
					if (xfr_export)
					{
						curSeries->muzzleVelocities.append(rows.at(1).toInt());
						qDebug() << "muzzleVelocities +=" << rows.at(1).toInt();

						if (curSeries->muzzleVelocities.size() == 1)
						{
							curSeries->velocityUnits = rows.at(2);
							qDebug() << "velocityUnits =" << curSeries->velocityUnits;
						}
					}
					else
					{
						curSeries->muzzleVelocities.append(rows.at(2).toInt());
						qDebug() << "muzzleVelocities +=" << rows.at(2).toInt();

						if (curSeries->muzzleVelocities.size() == 1)
						{
							curSeries->velocityUnits = rows.at(3);
							qDebug() << "velocityUnits =" << curSeries->velocityUnits;
						}
					}
				}
			}
		}

		i++;
	}

	// XFR export files do not include series numbers, so iterate through and set the seriesNum's
	for (i = 0; i < allSeries.size(); i++)
	{
		ChronoSeries *series = allSeries.at(i);
		series->seriesNum = i + 1;
		qDebug() << "Setting" << series->name << "to" << series->seriesNum;
	}

	return allSeries;
}

QList<ChronoSeries*> ChronographParsers::extractProChronoSeries(QTextStream &csv)
{
	QList<ChronoSeries*> allSeries;
	ChronoSeries *curSeries = new ChronoSeries();
	curSeries->isValid = true;
	curSeries->deleted = false;
	curSeries->seriesNum = -1;
	curSeries->velocityUnits = "ft/s";

	int i = 0;
	while (!csv.atEnd())
	{
		QString line = csv.readLine();

		// ProChrono uses comma (,) as delimeter
		QStringList rows(line.split(","));

		// Trim whitespace from cells
		QMutableStringListIterator it(rows);
		while (it.hasNext())
		{
			it.next();
			it.setValue(it.value().trimmed());
		}

		qDebug() << "Line" << i << ":" << rows;

		if (rows.size() >= 9)
		{
			if (rows.at(0) == "Shot List")
			{
				// skip column headers
				qDebug() << "Skipping column headers";
			}
			else
			{
				bool ok = false;
				int index = rows.at(1).toInt(&ok);

				// If cell is a valid integer, the row is a shot entry
				if (ok)
				{
					if (index == 1)
					{
						// First shot in the series. End the previous series (if necessary) and start a new one.

						if (curSeries->muzzleVelocities.size() > 0)
						{
							qDebug() << "Adding curSeries to allSeries";

							allSeries.append(curSeries);
						}

						qDebug() << "Beginning new series";

						curSeries = new ChronoSeries();
						curSeries->isValid = true;
						curSeries->deleted = false;
						curSeries->seriesNum = -1;
						curSeries->name = new QLabel(rows.at(0));
						curSeries->velocityUnits = "ft/s";
					}

					if (curSeries->firstDate.isNull())
					{
						QStringList dateTime = rows.at(8).split(" ");
						if (dateTime.size() == 2)
						{
							curSeries->firstDate = dateTime.at(0);
							curSeries->firstTime = dateTime.at(1);
							qDebug() << "firstDate =" << curSeries->firstDate;
							qDebug() << "firstTime =" << curSeries->firstTime;
						}
						else
						{
							qDebug() << "Failed to split datetime cell:" << rows.at(8);
						}
					}

					curSeries->muzzleVelocities.append(rows.at(2).toInt());
					qDebug() << "muzzleVelocities +=" << rows.at(2).toInt();
				}
			}
		}

		i++;
	}

	// End of the file. Finish parsing the current series.
	qDebug() << "End of file";

	if (curSeries->muzzleVelocities.size() > 0)
	{
		qDebug() << "Adding curSeries to allSeries";

		allSeries.append(curSeries);
	}

	// ProChrono files list series in reverse order from newest to oldest. Iterate through and
	// set the seriesNum's accordingly.
	int seriesNum = 1;
	for (i = allSeries.size() - 1; i >= 0; i--)
	{
		ChronoSeries *series = allSeries.at(i);
		series->seriesNum = seriesNum;
		seriesNum++;
	}

	return allSeries;
}

QList<ChronoSeries*> ChronographParsers::extractProChronoSeries_format2(QTextStream &csv)
{
	QList<ChronoSeries*> allSeries;
	ChronoSeries *curSeries = new ChronoSeries();
	curSeries->isValid = true;
	curSeries->deleted = false;
	curSeries->seriesNum = -1;
	curSeries->velocityUnits = "ft/s";

	int i = 0;
	while (!csv.atEnd())
	{
		QString line = csv.readLine();

		// ProChrono uses comma (,) as delimeter
		QStringList rows(line.split(","));

		// Trim whitespace from cells
		QMutableStringListIterator it(rows);
		while (it.hasNext())
		{
			it.next();
			it.setValue(it.value().trimmed());
		}

		qDebug() << "Line" << i << ":" << rows;

		if (rows.at(0).contains("Shot"))
		{
			// skip column headers
			qDebug() << "Skipping column headers";
		}
		else
		{
			// If cell is a valid integer, parse the row as velocity data

			bool ok = false;
			rows.at(0).toInt(&ok);
			if (ok)
			{
				// End the previous series (if necessary) and start a new one

				if (curSeries->muzzleVelocities.size() > 0)
				{
					qDebug() << "Adding curSeries to allSeries";

					allSeries.append(curSeries);
				}

				qDebug() << "Beginning new series";

				curSeries = new ChronoSeries();
				curSeries->isValid = true;
				curSeries->deleted = false;
				curSeries->seriesNum = -1;
				curSeries->velocityUnits = "ft/s";

				// Series in the file are recorded newest first. We'll iterate through and name them at the end.

				for (int j = 0; j < rows.size(); j++)
				{
					bool ok = false;
					int veloc = rows.at(j).toInt(&ok);

					if (ok)
					{
						curSeries->muzzleVelocities.append(rows.at(j).toInt());
						qDebug() << "muzzleVelocities +=" << rows.at(j).toInt();
					}
					else
					{
						qDebug() << "Skipping velocity entry:" << rows.at(j);
					}
				}
			}
			else
			{
				QDateTime seriesDateTime;
				seriesDateTime = QDateTime::fromString(rows.at(0), "M/d/yyyy hh:mm:ss");
				if (seriesDateTime.isValid())
				{
					QStringList dateTime = rows.at(0).split(" ");
					if (dateTime.size() == 2)
					{
						curSeries->firstDate = dateTime.at(0);
						curSeries->firstTime = dateTime.at(1);
						qDebug() << "firstDate =" << curSeries->firstDate;
						qDebug() << "firstTime =" << curSeries->firstTime;
					}
				}
			}
		}

		i++;
	}

	// End of the file. Finish parsing the current series.
	qDebug() << "End of file";

	if (curSeries->muzzleVelocities.size() > 0)
	{
		qDebug() << "Adding curSeries to allSeries";

		allSeries.append(curSeries);
	}

	// ProChrono files list series in reverse order from newest to oldest. Iterate through and
	// set the seriesNum's and names accordingly.
	int seriesNum = 1;
	for (i = allSeries.size() - 1; i >= 0; i--)
	{
		ChronoSeries *series = allSeries.at(i);
		series->seriesNum = seriesNum;
		series->name = new QLabel(QString("Series %1").arg(seriesNum));
		seriesNum++;
	}

	return allSeries;
}

QList<ChronoSeries*> ChronographParsers::extractGarminSeries_xlsx(QXlsx::Document &xlsx)
{
	QList<ChronoSeries*> allSeries;
	
	int i = 0;
	foreach (QString sheetName, xlsx.sheetNames())
	{
		qDebug() << "Sheet: " << sheetName;
		QXlsx::AbstractSheet *curSheet = xlsx.sheet(sheetName);
		if (curSheet == NULL)
		{
			qDebug() << "Failed to get sheet, skipping...";
			continue;
		}
		
		curSheet->workbook()->setActiveSheet(i);
		
		QXlsx::Worksheet *worksheet = (QXlsx::Worksheet *)curSheet->workbook()->activeSheet();
		if (worksheet == NULL)
		{
			qDebug() << "Failed to set active sheet, skipping...";
			continue;
		}
		
		ChronoSeries *curSeries = new ChronoSeries();
		curSeries->isValid = false;
		curSeries->deleted = false;
		curSeries->seriesNum = i + 1;
		curSeries->firstDate = QString("-");
		curSeries->firstTime = QString("");
		
		qDebug() << "Series name:" << worksheet->read(1,1).toString();
		curSeries->name = new QLabel(worksheet->read(1, 1).toString());
		
		// Unit of measure
		if (worksheet->read(2, 2).toString().contains("FPS"))
		{
			curSeries->velocityUnits = "ft/s";
		}
		else
		{
			curSeries->velocityUnits = "m/s";
		}
		
		int maxRow = 0, maxCol = 0;
		worksheet->getFullCells(&maxRow, &maxCol);
		
		// Iterate through each row of the worksheet, looking for velocities
		int j;
		for (j = 3; j <= maxRow; j++)
		{
			bool ok_shot_id = false;
			int shot_id = worksheet->read(j, 1).toInt(&ok_shot_id);
			if (ok_shot_id)
			{
				// We found a row with an integer (shot ID) in the first column
				
				bool ok_veloc = false;
				QString veloc_str = worksheet->read(j, 2).toString();
				veloc_str.replace(",", "."); // handle international-formatted numbers
				double veloc = veloc_str.toFloat(&ok_veloc);
				if (ok_veloc)
				{
					curSeries->muzzleVelocities.append(veloc);
					qDebug() << "muzzleVelocities +=" << veloc;
				}
				else
				{
					qDebug() << "Skipping velocity entry:" << worksheet->read(j, 2);
				}
			}
			else
			{
				if (worksheet->read(j, 1).toString().compare("DATE") == 0)
				{
					// Date time
					QStringList dateTime = worksheet->read(j, 2).toString().split(" at ");
					if (dateTime.size() == 2)
					{
						curSeries->firstDate = dateTime.at(0);
						curSeries->firstTime = dateTime.at(1);
						qDebug() << "firstDate =" << curSeries->firstDate;
						qDebug() << "firstTime =" << curSeries->firstTime;
					}
					else
					{
						qDebug() << "Failed to split datetime cell:" << worksheet->read(j, 2);
					}
				}
			}
		}
		
		if (curSeries->muzzleVelocities.size() > 0)
		{
			qDebug() << "Adding curSeries to allSeries";
			curSeries->isValid = true;

			allSeries.append(curSeries);
		}
		
		i += 1;
	}

	return allSeries;
}

// https://stackoverflow.com/a/40229435
bool ChronographParsers::readCSVRow(QTextStream &in, QStringList *row)
{
	static const int delta[][5] = {
		//  ,    "   \n    ?  eof
		{   1,   2,  -1,   0,  -1  }, // 0: parsing (store char)
		{   1,   2,  -1,   0,  -1  }, // 1: parsing (store column)
		{   3,   4,   3,   3,  -2  }, // 2: quote entered (no-op)
		{   3,   4,   3,   3,  -2  }, // 3: parsing inside quotes (store char)
		{   1,   3,  -1,   0,  -1  }, // 4: quote exited (no-op)
		// -1: end of row, store column, success
		// -2: eof inside quotes
	};

	row->clear();

	if (in.atEnd())
		return false;

	int state = 0, t;
	char ch;
	QString cell;

	while (state >= 0) {

		if (in.atEnd())
			t = 4;
		else {
			in >> ch;
			if (ch == ',') t = 0;
			else if (ch == '\"') t = 1;
			else if (ch == '\n') t = 2;
			else t = 3;
		}

		state = delta[state][t];

		if (state == 0 || state == 3) {
			cell += ch;
		} else if (state == -1 || state == 1) {
			row->append(cell);
			cell = "";
		}

	}

	if (state == -2)
	{
		qDebug() << "End-of-file found while inside quotes.";
		return false;
	}

	return true;
}

QList<ChronoSeries*> ChronographParsers::extractGarminSeries_csv(QTextStream &csv)
{
	QList<ChronoSeries*> allSeries;
	ChronoSeries *curSeries = new ChronoSeries();
	curSeries->isValid = false;
	curSeries->deleted = false;
	curSeries->seriesNum = 1;
	curSeries->velocityUnits = "ft/s";
	curSeries->firstDate = QString("-");
	curSeries->firstTime = QString("");

	int i = 0;
	QStringList cols;
	while (readCSVRow(csv, &cols))
	{
		// Trim whitespace from cells
		QMutableStringListIterator it(cols);
		while (it.hasNext())
		{
			it.next();
			it.setValue(it.value().trimmed());
		}

		qDebug() << "Line" << i << ":" << cols;

		if (cols.size() >= 1)
		{
			// Series name in first row, first column
			if (i == 0)
			{
				qDebug() << "Series name:" << cols.at(0);
				curSeries->name = new QLabel(cols.at(0));
			}
			// Unit of measure in second row, second column
			else if (i == 1)
			{
				if (cols.at(1).contains("FPS"))
				{
					qDebug() << "Velocity units: ft/s";
					curSeries->velocityUnits = "ft/s";
				}
				else
				{
					qDebug() << "Velocity units: m/s";
					curSeries->velocityUnits = "m/s";
				}
			}
			// Date time
			else if (cols.at(0).compare("DATE") == 0)
			{
				curSeries->firstDate = cols.at(1);
				curSeries->firstTime = QString("");
				qDebug() << "firstDate =" << curSeries->firstDate;
				qDebug() << "firstTime =" << curSeries->firstTime;
			}
			// Look for shot velocity row
			else
			{
				bool ok_shot_id = false;
				int shot_id = cols.at(0).toInt(&ok_shot_id);
				if (ok_shot_id)
				{
					// We found a row with an integer (shot ID) in the first column
					
					bool ok_veloc = false;
					QString veloc_str = cols.at(1);
					veloc_str.replace(",", "."); // handle international-formatted numbers
					double veloc = veloc_str.toFloat(&ok_veloc);
					if (ok_veloc)
					{
						curSeries->muzzleVelocities.append(veloc);
						qDebug() << "muzzleVelocities +=" << veloc;
					}
					else
					{
						qDebug() << "Skipping velocity entry:" << cols.at(1);
					}
				}
			}
		}

		i++;
	}

	// End of the file. Finish parsing the current series.
	qDebug() << "End of file";
	
	// Ensure we have a valid Garmin series
	if (curSeries->muzzleVelocities.empty())
	{
		qDebug() << "Series has no velocities, returning invalid.";
		return allSeries;
	}

	allSeries.append(curSeries);

	return allSeries;
}

QList<ChronoSeries*> ChronographParsers::extractShotMarkerSeriesTar(QString path)
{
	QList<ChronoSeries*> allSeries;
	ChronoSeries *curSeries = new ChronoSeries();
	QTemporaryDir tempDir;
	int ret;

	if (!tempDir.isValid())
	{
		qDebug() << "Temp directory is NOT valid";
	}

	qDebug() << "Temporary directory:" << tempDir.path();

	QFile rf(path);
	if (!rf.open(QIODevice::ReadOnly))
	{
		qDebug() << "Failed to open ShotMarker .tar file:" << path;
		return allSeries;
	}

	ret = untar(rf, tempDir.path());
	if (ret)
	{
		qDebug() << "Error while extracting ShotMarker .tar file:" << path;
		return allSeries;
	}

	QDir dir(tempDir.path());
	QStringList stringFiles = dir.entryList(QStringList() << "*.z", QDir::Files);

	qDebug() << "iterating over files:";
	int seriesNum = 1;
	foreach (QString filename, stringFiles)
	{
		QString path(tempDir.path());
		path.append("/");
		path.append(filename);
		qDebug() << path;

		QFile file(path);
		file.open(QIODevice::ReadOnly);
		QByteArray buf = file.readAll();
		qDebug() << "file:" << path << ", size:" << buf.size();

		// 1mb ought to be enough for anybody!
		unsigned char *destBuf = (unsigned char *)malloc(1024 * 1024);
		qDebug() << "malloc returned" << (void *)destBuf;
		if (destBuf == NULL)
		{
			qDebug() << "malloc returned NULL! skipping... but we should really throw an exception here.";
			continue;
		}

		mz_ulong uncomp_len = 1024 * 1024;
		qDebug() << "calling uncompress 1 with destBuf=" << (void *)destBuf << ", uncomp_len=" << uncomp_len << ", buf.data()=" << buf.data() << ", buf.size()=" << buf.size();
		ret = uncompress(destBuf, &uncomp_len, (const unsigned char *)buf.data(), buf.size());

		qDebug() << "output size:" << uncomp_len;
		if (ret != MZ_OK)
		{
			qDebug() << "Failed to uncompress, skipping..." << path;
			free(destBuf);
			continue;
		}

		QByteArray ba = QByteArray::fromRawData((const char *)destBuf, uncomp_len);

		QJsonParseError parseError;
		QJsonDocument jsonDoc;
		jsonDoc = QJsonDocument::fromJson(ba, &parseError);

		if (parseError.error != QJsonParseError::NoError)
		{
			qDebug() << "JSON parse error, skipping... at" << parseError.offset << ":" << parseError.errorString();
			free(destBuf);
			continue;
		}

		QJsonObject jsonObj = jsonDoc.object();

		qDebug() << "Beginning new series";

		curSeries = new ChronoSeries();
		curSeries->isValid = false;
		curSeries->seriesNum = seriesNum;
		qDebug() << "name =" << jsonObj["name"].toString();
		curSeries->name = new QLabel(jsonObj["name"].toString());
		curSeries->velocityUnits = "ft/s";
		curSeries->deleted = false;
		QDateTime dateTime;
		dateTime.setMSecsSinceEpoch(jsonObj["ts"].toVariant().toULongLong());
		curSeries->firstDate = dateTime.date().toString(Qt::TextDate);
		curSeries->firstTime = dateTime.time().toString(Qt::TextDate);

		qDebug() << "setting date =" << curSeries->firstDate << " time =" << curSeries->firstTime << "from ts" << jsonObj["ts"].toVariant().toULongLong();

		foreach (const QJsonValue& shot, jsonObj["shots"].toArray())
		{
			// convert from m/s to ft/s
			int velocity = shot["v"].toDouble() * 1.0936133 * 3; // the result is cast to an int

			if (shot["hidden"].toBool())
			{
				qDebug() << "ignoring hidden shot" << shot["display_text"];
			}
			else if (shot["sighter"].toBool())
			{
				qDebug() << "ignoring sighter shot" << shot["display_text"];
			}
			else
			{
				qDebug() << "adding velocity" << velocity << "from m/s:" << shot["v"].toDouble();
				curSeries->muzzleVelocities.append(velocity);
			}
		}

		// Finish parsing the current series.
		qDebug() << "End of JSON";

		if (curSeries->muzzleVelocities.size() > 0)
		{
			qDebug() << "Adding curSeries to allSeries";

			allSeries.append(curSeries);
		}

		free(destBuf);

		seriesNum++;
	}

	return allSeries;
}

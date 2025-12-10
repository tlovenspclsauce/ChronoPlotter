#include "FileSelectionHandlers.h"
#include "PowderTest.h"
#include "ChronographParsers.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <QTextStream>
#include <QDebug>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>

#include "xlsxdocument.h"

using namespace Powder;

QList<ChronoSeries*> FileSelectionHandlers::selectLabRadarDirectory(
	QWidget *parent,
	const QString &prevDir,
	QString *outDir)
{
	qDebug() << "selectLabRadarDirectory";
	qDebug() << "Previous directory:" << prevDir;

	QString path = QFileDialog::getExistingDirectory(parent, "Select directory", prevDir);
	*outDir = path;

	qDebug() << "Selected directory:" << path;

	QList<ChronoSeries*> seriesData;

	if (path.isEmpty())
	{
		qDebug() << "User didn't select a directory, bail";
		return seriesData;
	}

	// Look for LabRadar data
	QDir lbrPath(path);
	lbrPath.setPath(lbrPath.filePath("LBR"));
	if (lbrPath.exists())
	{
		path = lbrPath.path();
		qDebug() << "Detected LabRadar directory" << path << ". Using that directory instead.";
	}

	QDir trkPath(path);
	trkPath.setPath(trkPath.filePath("TRK"));
	if (trkPath.exists())
	{
		trkPath.setPath(trkPath.filePath("../.."));
		path = trkPath.canonicalPath();
		qDebug() << "Detected LabRadar directory" << path << ". Using one directory level up instead.";
	}

	qDebug() << "path:" << path;

	/* Enumerate the LabRadar directory */
	QRegularExpression re;
	re.setPattern("^SR\\d\\d\\d\\d.*");

	QDir dir(path);
	QStringList items = dir.entryList(QStringList(), QDir::AllDirs | QDir::NoDotAndDotDot);

	foreach (QString fileName, items)
	{
		qDebug() << "Entry:" << fileName;
		if (re.match(fileName).hasMatch())
		{
			qDebug() << "Detected LabRadar series directory" << fileName;

			QString seriesPath(dir.filePath(fileName));
			QDir seriesDir(seriesPath);
			QStringList csvItems = seriesDir.entryList(QStringList() << "* Report.csv", QDir::Files | QDir::NoDotAndDotDot);
			QString csvFileName = csvItems.at(0);

			qDebug() << "CSV file:" << csvFileName;

			QFile csvFile(seriesDir.filePath(csvFileName));
			csvFile.open(QIODevice::ReadOnly);
			QTextStream csv(&csvFile);

			ChronoSeries *series = ChronographParsers::extractLabRadarSeries(csv);

			if (!series->isValid)
			{
				qDebug() << "Invalid series, skipping...";
				continue;
			}

			series->enabled = new QCheckBox();
			series->enabled->setChecked(true);

			series->name = new QLabel(fileName);

			series->chargeWeight = new QDoubleSpinBox();
			series->chargeWeight->setDecimals(2);
			series->chargeWeight->setSingleStep(0.1);
			series->chargeWeight->setMaximum(1000000);
			series->chargeWeight->setMinimumWidth(100);
			series->chargeWeight->setMaximumWidth(100);

			seriesData.append(series);

			csvFile.close();
		}
	}

	/* We're finished enumerating the directory */
	if (seriesData.empty())
	{
		qDebug() << "Didn't find any chrono data in this directory, bail";

		QMessageBox *msg = new QMessageBox();
		msg->setIcon(QMessageBox::Critical);
		msg->setText(QString("Unable to find LabRadar data in '%1'").arg(path));
		msg->setWindowTitle("Error");
		msg->exec();
	}
	else
	{
		qDebug() << "Detected LabRadar directory" << path;

		QMessageBox *msg = new QMessageBox();
		msg->setIcon(QMessageBox::Information);
		msg->setText(QString("Detected LabRadar data\n\nUsing '%1'").arg(path));
		msg->setWindowTitle("Success");
		msg->exec();
	}

	return seriesData;
}

QList<ChronoSeries*> FileSelectionHandlers::selectMagnetoSpeedFile(
	QWidget *parent,
	const QString &prevDir,
	QString *outDir)
{
	qDebug() << "selectMagnetoSpeedFile";
	qDebug() << "Previous directory:" << prevDir;

	QString path = QFileDialog::getOpenFileName(parent, "Select file", prevDir, "CSV files (*.csv)");
	*outDir = path;

	qDebug() << "Selected file:" << path;

	QList<ChronoSeries*> seriesData;

	if (path.isEmpty())
	{
		qDebug() << "User didn't select a file, bail";
		return seriesData;
	}

	QFile csvFile(path);
	csvFile.open(QIODevice::ReadOnly);
	QTextStream csv(&csvFile);

	QList<ChronoSeries*> allSeries = ChronographParsers::extractMagnetoSpeedSeries(csv);

	qDebug() << "Got allSeries from ExtractMagnetoSpeedSeries with size" << allSeries.size();

	if (!allSeries.empty())
	{
		qDebug() << "Detected MagnetoSpeed file";

		for (int i = 0; i < allSeries.size(); i++)
		{
			ChronoSeries *series = allSeries.at(i);

			series->enabled = new QCheckBox();
			series->enabled->setChecked(true);

			series->chargeWeight = new QDoubleSpinBox();
			series->chargeWeight->setDecimals(2);
			series->chargeWeight->setSingleStep(0.1);
			series->chargeWeight->setMaximum(1000000);
			series->chargeWeight->setMinimumWidth(100);
			series->chargeWeight->setMaximumWidth(100);

			seriesData.append(series);
		}
	}

	csvFile.close();

	/* We're finished parsing the file */
	if (seriesData.empty())
	{
		qDebug() << "Didn't find any chrono data in this file, bail";

		QMessageBox *msg = new QMessageBox();
		msg->setIcon(QMessageBox::Critical);
		msg->setText(QString("Unable to find MagnetoSpeed data in '%1'").arg(path));
		msg->setWindowTitle("Error");
		msg->exec();
	}
	else
	{
		qDebug() << "Detected MagnetoSpeed file" << path;

		QMessageBox *msg = new QMessageBox();
		msg->setIcon(QMessageBox::Information);
		msg->setText(QString("Detected MagnetoSpeed data\n\nUsing '%1'").arg(path));
		msg->setWindowTitle("Success");
		msg->exec();
	}

	return seriesData;
}

QList<ChronoSeries*> FileSelectionHandlers::selectProChronoFile(
	QWidget *parent,
	const QString &prevDir,
	QString *outDir)
{
	qDebug() << "selectProChronoFile";
	qDebug() << "Previous directory:" << prevDir;

	QString path = QFileDialog::getOpenFileName(parent, "Select file", prevDir, "CSV files (*.csv)");
	*outDir = path;

	qDebug() << "Selected file:" << path;

	QList<ChronoSeries*> seriesData;

	if (path.isEmpty())
	{
		qDebug() << "User didn't select a file, bail";
		return seriesData;
	}

	QFile csvFile(path);
	csvFile.open(QIODevice::ReadOnly);
	QTextStream csv(&csvFile);

	// Test which format this ProChrono file is
	QString line = csv.readLine();
	csv.seek(0);

	QList<ChronoSeries*> allSeries;

	if (line.startsWith("Shot 1"))
	{
		qDebug() << "Detected ProChrono format 2";
		allSeries = ChronographParsers::extractProChronoSeries_format2(csv);
	}
	else
	{
		qDebug() << "Detected ProChrono format 1";
		allSeries = ChronographParsers::extractProChronoSeries(csv);
	}

	qDebug() << "Got allSeries from ExtractProChronoSeries with size" << allSeries.size();

	if (!allSeries.empty())
	{
		qDebug() << "Detected ProChrono file";

		for (int i = 0; i < allSeries.size(); i++)
		{
			ChronoSeries *series = allSeries.at(i);

			series->enabled = new QCheckBox();
			series->enabled->setChecked(true);

			series->chargeWeight = new QDoubleSpinBox();
			series->chargeWeight->setDecimals(2);
			series->chargeWeight->setSingleStep(0.1);
			series->chargeWeight->setMaximum(1000000);
			series->chargeWeight->setMinimumWidth(100);
			series->chargeWeight->setMaximumWidth(100);

			seriesData.append(series);
		}
	}

	csvFile.close();

	/* We're finished parsing the file */
	if (seriesData.empty())
	{
		qDebug() << "Didn't find any chrono data in this file, bail";

		QMessageBox *msg = new QMessageBox();
		msg->setIcon(QMessageBox::Critical);
		msg->setText(QString("Unable to find ProChrono data in '%1'").arg(path));
		msg->setWindowTitle("Error");
		msg->exec();
	}
	else
	{
		qDebug() << "Detected ProChrono file" << path;

		QMessageBox *msg = new QMessageBox();
		msg->setIcon(QMessageBox::Information);
		msg->setText(QString("Detected ProChrono data\n\nUsing '%1'").arg(path));
		msg->setWindowTitle("Success");
		msg->exec();
	}

	return seriesData;
}

QList<ChronoSeries*> FileSelectionHandlers::selectGarminFile(
	QWidget *parent,
	const QString &prevDir,
	QString *outDir)
{
	qDebug() << "selectGarminFile";
	qDebug() << "Previous directory:" << prevDir;

	QString path = QFileDialog::getOpenFileName(parent, "Select file", prevDir, "Garmin files (*.xlsx *.csv)");
	*outDir = path;

	qDebug() << "Selected file:" << path;

	QList<ChronoSeries*> seriesData;

	if (path.isEmpty())
	{
		qDebug() << "User didn't select a file, bail";
		return seriesData;
	}

	QList<ChronoSeries*> allSeries;

	if (path.endsWith(".xlsx", Qt::CaseInsensitive))
	{
		qDebug() << "Garmin XLSX file";

		QXlsx::Document xlsx(path);
		xlsx.load();

		qDebug() << "Loaded xlsx doc. sheets: " << xlsx.sheetNames();

		allSeries = ChronographParsers::extractGarminSeries_xlsx(xlsx);
	}
	else if (path.endsWith(".csv", Qt::CaseInsensitive))
	{
		qDebug() << "Garmin CSV file";

		QFile csvFile(path);
		csvFile.open(QIODevice::ReadOnly | QIODevice::Text);
		QTextStream csv(&csvFile);

		allSeries = ChronographParsers::extractGarminSeries_csv(csv);
	}
	else
	{
		qDebug() << "Garmin unsupported file, bailing...";

		QMessageBox *msg = new QMessageBox();
		msg->setIcon(QMessageBox::Critical);
		msg->setText(QString("Only Garmin .XLSX and .CSV files are supported.\n\nSelected: '%1'").arg(path));
		msg->setWindowTitle("Error");
		msg->exec();

		return seriesData;
	}

	qDebug() << "Got allSeries with size" << allSeries.size();

	if (!allSeries.empty())
	{
		qDebug() << "Detected Garmin file";

		for (int i = 0; i < allSeries.size(); i++)
		{
			ChronoSeries *series = allSeries.at(i);

			series->enabled = new QCheckBox();
			series->enabled->setChecked(true);

			series->chargeWeight = new QDoubleSpinBox();
			series->chargeWeight->setDecimals(2);
			series->chargeWeight->setSingleStep(0.1);
			series->chargeWeight->setMaximum(1000000);
			series->chargeWeight->setMinimumWidth(100);
			series->chargeWeight->setMaximumWidth(100);

			seriesData.append(series);
		}
	}

	/* We're finished parsing the file */
	if (seriesData.empty())
	{
		qDebug() << "Didn't find any chrono data in this file, bail";

		QMessageBox *msg = new QMessageBox();
		msg->setIcon(QMessageBox::Critical);
		msg->setText(QString("Unable to find Garmin data in '%1'").arg(path));
		msg->setWindowTitle("Error");
		msg->exec();
	}
	else
	{
		qDebug() << "Detected Garmin file" << path;

		QMessageBox *msg = new QMessageBox();
		msg->setIcon(QMessageBox::Information);
		msg->setText(QString("Detected Garmin data\n\nUsing '%1'").arg(path));
		msg->setWindowTitle("Success");
		msg->exec();
	}

	return seriesData;
}

QList<ChronoSeries*> FileSelectionHandlers::selectShotMarkerFile(
	QWidget *parent,
	const QString &prevDir,
	QString *outDir)
{
	qDebug() << "selectShotMarkerFile";
	qDebug() << "Previous directory:" << prevDir;

	QString path = QFileDialog::getOpenFileName(parent, "Select file", prevDir, "ShotMarker files (*.tar)");
	*outDir = path;

	qDebug() << "Selected file:" << path;

	QList<ChronoSeries*> seriesData;

	if (path.isEmpty())
	{
		qDebug() << "User didn't select a file, bail";
		return seriesData;
	}

	QList<ChronoSeries*> allSeries;

	if (path.endsWith(".tar"))
	{
		qDebug() << "ShotMarker .tar bundle";

		allSeries = ChronographParsers::extractShotMarkerSeriesTar(path);
	}
	else
	{
		qDebug() << "ShotMarker .csv export, bailing";

		QMessageBox *msg = new QMessageBox();
		msg->setIcon(QMessageBox::Critical);
		msg->setText(QString("Only ShotMarker .tar files are supported for velocity data.\n\nSelected: '%1'").arg(path));
		msg->setWindowTitle("Error");
		msg->exec();

		return seriesData;
	}

	qDebug() << "Got allSeries with size" << allSeries.size();

	if (!allSeries.empty())
	{
		qDebug() << "Detected ShotMarker file";

		for (int i = 0; i < allSeries.size(); i++)
		{
			ChronoSeries *series = allSeries.at(i);

			series->enabled = new QCheckBox();
			series->enabled->setChecked(true);

			series->chargeWeight = new QDoubleSpinBox();
			series->chargeWeight->setDecimals(2);
			series->chargeWeight->setSingleStep(0.1);
			series->chargeWeight->setMaximum(1000000);
			series->chargeWeight->setMinimumWidth(100);
			series->chargeWeight->setMaximumWidth(100);

			seriesData.append(series);
		}
	}

	/* We're finished parsing the file */
	if (seriesData.empty())
	{
		qDebug() << "Didn't find any shot data in this file, bail";

		QMessageBox *msg = new QMessageBox();
		msg->setIcon(QMessageBox::Critical);
		msg->setText(QString("Unable to find ShotMarker data in '%1'").arg(path));
		msg->setWindowTitle("Error");
		msg->exec();
	}
	else
	{
		qDebug() << "Detected ShotMarker file" << path;

		QMessageBox *msg = new QMessageBox();
		msg->setIcon(QMessageBox::Information);
		msg->setText(QString("Detected ShotMarker data\n\nUsing '%1'").arg(path));
		msg->setWindowTitle("Success");
		msg->exec();
	}

	return seriesData;
}

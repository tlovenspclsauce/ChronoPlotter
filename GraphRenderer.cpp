#include "GraphRenderer.h"
#include "PowderTest.h"
#include "ChronoPlotter.h"

#include "qcustomplot/qcustomplot.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <algorithm>
#include <numeric>

using namespace Powder;

static bool ChargeWeightComparator(ChronoSeries *one, ChronoSeries *two)
{
	return (one->chargeWeight->value() < two->chargeWeight->value());
}

void GraphRenderer::renderGraph(
	QWidget *parent,
	const QList<ChronoSeries*> &seriesData,
	bool displayGraphPreview,
	const GraphOptions &options,
	const QString &prevSaveDir,
	QString *outSaveDir,
	GraphPreview **outGraphPreview)
{
	qDebug() << "renderGraph displayGraphPreview =" << displayGraphPreview;

	/* Validate series before continuing */
	int numEnabled = 0;
	if (!validateSeries(parent, seriesData, &numEnabled))
	{
		return;
	}

	if (numEnabled < 2)
	{
		qDebug() << "Only" << numEnabled << "series enabled, bailing";

		QMessageBox *msg = new QMessageBox();
		msg->setIcon(QMessageBox::Critical);
		msg->setText("At least two series are required to graph!");
		msg->setWindowTitle("Error");
		msg->exec();
		return;
	}

	QCustomPlot *customPlot = new QCustomPlot();
	customPlot->setGeometry(40, 40, 1440, 625);
	customPlot->setAntialiasedElements(QCP::aeAll);

	/* Make a copy of the subset of data actually being graphed */
	QList<ChronoSeries*> seriesToGraph = getEnabledSeries(seriesData);

	/* Sort the data by charge weight */
	std::sort(seriesToGraph.begin(), seriesToGraph.end(), ChargeWeightComparator);

	/* Check if any charge weights are duplicated */
	int xAxisSpacing = options.xAxisSpacingIndex;
	if (!checkDuplicateChargeWeights(parent, seriesToGraph, options.xAxisSpacingIndex, &xAxisSpacing))
	{
		return;
	}

	/* Collect the data to graph */
	QSharedPointer<QCPAxisTickerText> textTicker(new QCPAxisTickerText);

	QVector<double> xPoints;
	QVector<double> yPoints;
	QVector<double> xAvgPoints;
	QVector<double> yAvgPoints;
	QVector<double> yError;
	QVector<double> allXPoints;
	QVector<double> allYPoints;

	for (int i = 0; i < seriesToGraph.size(); i++)
	{
		ChronoSeries *series = seriesToGraph.at(i);

		double chargeWeight = series->chargeWeight->value();

		qDebug() << QString("Series %1 (%2 gr)").arg(series->seriesNum).arg(chargeWeight);
		qDebug() << series->muzzleVelocities;

		int totalShots = series->muzzleVelocities.size();
		double mean = std::accumulate(series->muzzleVelocities.begin(), series->muzzleVelocities.end(), 0.0) / static_cast<double>(totalShots);
		double stdev = sampleStdev(series->muzzleVelocities);

		qDebug() << "Total shots:" << totalShots;
		qDebug() << "Mean:" << mean;
		qDebug() << "Stdev:" << stdev;

		if (xAxisSpacing == CONSTANT)
		{
			xAvgPoints.push_back(i);
		}
		else
		{
			xAvgPoints.push_back(chargeWeight);
		}
		yAvgPoints.push_back(mean);

		if (options.graphType == SCATTER)
		{
			for (int j = 0; j < totalShots; j++)
			{
				if (xAxisSpacing == CONSTANT)
				{
					xPoints.push_back(i);
					allXPoints.push_back(i);
				}
				else
				{
					xPoints.push_back(chargeWeight);
					allXPoints.push_back(chargeWeight);
				}
				yPoints.push_back(series->muzzleVelocities.at(j));
				allYPoints.push_back(series->muzzleVelocities.at(j));
			}
		}
		else
		{
			for (int j = 0; j < totalShots; j++)
			{
				if (xAxisSpacing == CONSTANT)
				{
					allXPoints.push_back(i);
				}
				else
				{
					allXPoints.push_back(chargeWeight);
				}
				allYPoints.push_back(series->muzzleVelocities.at(j));
			}

			if (xAxisSpacing == CONSTANT)
			{
				xPoints.push_back(i);
			}
			else
			{
				xPoints.push_back(chargeWeight);
			}
			yPoints.push_back(mean);
			yError.push_back(stdev);
		}

		if (xAxisSpacing == CONSTANT)
		{
			textTicker->addTick(i, QString::number(series->chargeWeight->value()));
		}
		else
		{
			textTicker->addTick(chargeWeight, QString::number(series->chargeWeight->value()));
		}
	}

	/* Create average line */
	QPen avgLinePen(Qt::SolidLine);
	QColor avgLineColor("#1c57eb");
	avgLineColor.setAlphaF(0.65);
	avgLinePen.setColor(avgLineColor);
	avgLinePen.setWidthF(1.5);

	QCPGraph *averageLine = customPlot->addGraph();
	averageLine->setData(xAvgPoints, yAvgPoints);
	averageLine->setScatterStyle(QCPScatterStyle::ssNone);
	averageLine->setPen(avgLinePen);

	/* Create scatter plot */
	QCPGraph *scatterPlot = customPlot->addGraph();
	scatterPlot->setData(xPoints, yPoints);
	scatterPlot->rescaleAxes();
	scatterPlot->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, QColor("#0536b0"), 6.0));
	scatterPlot->setLineStyle(QCPGraph::lsNone);

	/* Draw SD error bars if necessary */
	if (options.graphType == LINE_SD)
	{
		QCPErrorBars *errorBars = new QCPErrorBars(customPlot->xAxis, customPlot->yAxis);
		errorBars->setData(yError);
		errorBars->setDataPlottable(averageLine);
		errorBars->rescaleAxes();

		customPlot->xAxis->grid()->setVisible(false);
	}

	/* Draw trend line if necessary */
	if (options.showTrend)
	{
		std::vector<double> res = GetLinearFit(allXPoints, allYPoints);
		qDebug() << "linear fit:" << res[0] << res[1];

		QVector<double> xTrendPoints;
		QVector<double> yTrendPoints;
		xTrendPoints.push_back(allXPoints.first());
		yTrendPoints.push_back(res[1] + (allXPoints.first() * res[0]));
		xTrendPoints.push_back(allXPoints.last());
		yTrendPoints.push_back(res[1] + (allXPoints.last() * res[0]));

		Qt::PenStyle lineType;
		if (options.trendLineType == SOLID_LINE)
		{
			lineType = Qt::SolidLine;
		}
		else
		{
			lineType = Qt::DashLine;
		}

		QPen trendLinePen(lineType);
		QColor trendLineColor(Qt::red);
		trendLineColor.setAlphaF(0.65);
		trendLinePen.setColor(trendLineColor);
		trendLinePen.setWidthF(1.5);

		QCPGraph *trendLine = customPlot->addGraph();
		trendLine->setData(xTrendPoints, yTrendPoints);
		trendLine->setScatterStyle(QCPScatterStyle::ssNone);
		trendLine->setPen(trendLinePen);
	}

	/* Configure rest of the graph */
	QString weightUnits2 = getWeightUnit(options.weightUnitsIndex);
	QString velocityUnits2 = getVelocityUnit(options.velocityUnitsIndex);

	QCPTextElement *title = new QCPTextElement(customPlot);
	title->setText(QString("\n%1").arg(options.graphTitle));
	title->setFont(QFont("DejaVu Sans", scaleFontSize(24)));
	title->setTextColor(QColor("#4d4d4d"));
	customPlot->plotLayout()->insertRow(0);
	customPlot->plotLayout()->addElement(0, 0, title);

	QCPTextElement *subtitle = new QCPTextElement(customPlot);
	QStringList subtitleText;
	subtitleText << options.rifle << options.propellant << options.projectile << options.brass << options.primer << options.weather;
	subtitle->setText(StringListJoin(subtitleText, ", ") + "\n");
	subtitle->setFont(QFont("DejaVu Sans", scaleFontSize(12)));
	subtitle->setTextColor(QColor("#4d4d4d"));
	customPlot->plotLayout()->insertRow(1);
	customPlot->plotLayout()->addElement(1, 0, subtitle);

	QPen gridPen(Qt::SolidLine);
	gridPen.setColor("#d9d9d9");

	QPen axisBasePen(Qt::SolidLine);
	axisBasePen.setColor("#d9d9d9");
	axisBasePen.setWidth(2);

	customPlot->xAxis->setLabel(QString("Powder charge (%1)").arg(weightUnits2));
	customPlot->xAxis->scaleRange(1.1);
	customPlot->xAxis->setTicker(textTicker);
	customPlot->xAxis->setTickLabelFont(QFont("DejaVu Sans", scaleFontSize(9)));
	customPlot->xAxis->setTickLabelColor(QColor("#4d4d4d"));
	customPlot->xAxis->setLabelFont(QFont("DejaVu Sans", scaleFontSize(12)));
	customPlot->xAxis->setLabelColor(QColor("#4d4d4d"));
	customPlot->xAxis->grid()->setZeroLinePen(Qt::NoPen);
	customPlot->xAxis->grid()->setPen(gridPen);
	customPlot->xAxis->setBasePen(axisBasePen);
	customPlot->xAxis->setTickPen(Qt::NoPen);
	customPlot->xAxis->setSubTickPen(Qt::NoPen);
	customPlot->xAxis->setLabelPadding(13);
	customPlot->xAxis->setPadding(20);

	customPlot->xAxis2->setBasePen(axisBasePen);
	customPlot->xAxis2->setTickPen(Qt::NoPen);
	customPlot->xAxis2->setSubTickPen(Qt::NoPen);
	customPlot->xAxis2->setPadding(20);

	customPlot->yAxis->setLabel(QString("Velocity (%1)").arg(velocityUnits2));
	customPlot->yAxis->scaleRange(1.3);
	customPlot->yAxis->setTickLabelFont(QFont("DejaVu Sans", scaleFontSize(9)));
	customPlot->yAxis->setTickLabelColor(QColor("#4d4d4d"));
	customPlot->yAxis->setLabelFont(QFont("DejaVu Sans", scaleFontSize(12)));
	customPlot->yAxis->setLabelColor(QColor("#4d4d4d"));
	customPlot->yAxis->grid()->setZeroLinePen(Qt::NoPen);
	customPlot->yAxis->grid()->setPen(gridPen);
	customPlot->yAxis->setBasePen(axisBasePen);
	customPlot->yAxis->setTickPen(Qt::NoPen);
	customPlot->yAxis->setSubTickPen(Qt::NoPen);
	customPlot->yAxis->setLabelPadding(20);
	customPlot->yAxis->setPadding(20);

	customPlot->yAxis2->setBasePen(axisBasePen);
	customPlot->yAxis2->setTickPen(Qt::NoPen);
	customPlot->yAxis2->setSubTickPen(Qt::NoPen);
	customPlot->yAxis2->setPadding(20);

	customPlot->yAxis->ticker()->setTickCount(6);
	customPlot->axisRect()->setupFullAxesBox();

	// Render the graph off-screen
	QPixmap picture(QSize(1440, 625));
	QCPPainter painter(&picture);
	customPlot->toPainter(&painter, 1440, 625);

	/* Generate bounding boxes and text annotations */
	addAnnotations(customPlot, seriesToGraph, options);

	if (displayGraphPreview)
	{
		qDebug() << "Showing graph preview";

		QPixmap preview = customPlot->toPixmap(1440, 625, 2.0);

		if (*outGraphPreview)
		{
			(*outGraphPreview)->deleteLater();
		}

		qDebug() << "xPoints:" << xPoints;
		qDebug() << "yPoints:" << yPoints;
		qDebug() << "allXPoints:" << allXPoints;
		qDebug() << "allYPoints:" << allYPoints;

		*outGraphPreview = new GraphPreview(preview);
	}
	else
	{
		saveGraphToFile(parent, customPlot, options.graphTitle, prevSaveDir, outSaveDir);
	}
}

bool GraphRenderer::validateSeries(
	QWidget *parent,
	const QList<ChronoSeries*> &seriesData,
	int *outNumEnabled)
{
	int numEnabled = 0;
	for (int i = 0; i < seriesData.size(); i++)
	{
		ChronoSeries *series = seriesData.at(i);
		if ((!series->deleted) && series->enabled->isChecked())
		{
			numEnabled += 1;
			if (series->chargeWeight->value() == 0)
			{
				qDebug() << series->name->text() << "is missing charge weight, bailing";

				QMessageBox *msg = new QMessageBox();
				msg->setIcon(QMessageBox::Critical);
				msg->setText(QString("'%1' is missing charge weight!").arg(series->name->text()));
				msg->setWindowTitle("Error");
				msg->exec();
				return false;
			}
			else if (series->muzzleVelocities.size() == 0)
			{
				qDebug() << series->name->text() << "is missing velocities, bailing";

				QMessageBox *msg = new QMessageBox();
				msg->setIcon(QMessageBox::Critical);
				msg->setText(QString("'%1' is missing velocities!").arg(series->name->text()));
				msg->setWindowTitle("Error");
				msg->exec();
				return false;
			}
		}
	}

	*outNumEnabled = numEnabled;
	return true;
}

QList<ChronoSeries*> GraphRenderer::getEnabledSeries(const QList<ChronoSeries*> &seriesData)
{
	QList<ChronoSeries*> seriesToGraph;
	for (int i = 0; i < seriesData.size(); i++)
	{
		ChronoSeries *series = seriesData.at(i);

		if ((!series->deleted) && series->enabled->isChecked())
		{
			seriesToGraph.append(series);
		}
		else
		{
			qDebug() << "Series" << series->seriesNum << "is unchecked, skipping...";
		}
	}

	return seriesToGraph;
}

bool GraphRenderer::checkDuplicateChargeWeights(
	QWidget *parent,
	const QList<ChronoSeries*> &seriesToGraph,
	int xAxisSpacingIndex,
	int *outXAxisSpacing)
{
	if (xAxisSpacingIndex == PROPORTIONAL)
	{
		double lastChargeWeight = 0;
		for (int i = 0; i < seriesToGraph.size(); i++)
		{
			ChronoSeries *series = seriesToGraph.at(i);
			double chargeWeight = series->chargeWeight->value();

			if (chargeWeight == lastChargeWeight)
			{
				qDebug() << "Duplicate charge weight detected" << chargeWeight;

				QMessageBox::StandardButton reply;
				reply = QMessageBox::question(parent, "Duplicate charge weights", 
					"Duplicate charge weights detected. Switching graph to constant spacing mode.", 
					QMessageBox::Ok | QMessageBox::Cancel);

				if (reply == QMessageBox::Ok)
				{
					qDebug() << "Set x-axis spacing to constant";
					*outXAxisSpacing = CONSTANT;
					return true;
				}
				else
				{
					qDebug() << "User cancel, bailing out";
					return false;
				}
			}

			lastChargeWeight = chargeWeight;
		}
	}
	else
	{
		qDebug() << "Constant x-axis spacing selected, skipping duplicate check";
	}

	*outXAxisSpacing = xAxisSpacingIndex;
	return true;
}

void GraphRenderer::addAnnotations(
	QCustomPlot *customPlot,
	const QList<ChronoSeries*> &seriesToGraph,
	const GraphOptions &options)
{
	bool prevMeanSet = false;
	double prevMean = 0;

	for (int i = 0; i < seriesToGraph.size(); i++)
	{
		ChronoSeries *series = seriesToGraph.at(i);

		double chargeWeight = series->chargeWeight->value();

		int totalShots = series->muzzleVelocities.size();
		double velocityMin = *std::min_element(series->muzzleVelocities.begin(), series->muzzleVelocities.end());
		double velocityMax = *std::max_element(series->muzzleVelocities.begin(), series->muzzleVelocities.end());
		double mean = std::accumulate(series->muzzleVelocities.begin(), series->muzzleVelocities.end(), 0.0) / static_cast<double>(totalShots);
		int es = velocityMax - velocityMin;
		double stdev = sampleStdev(series->muzzleVelocities);
		QStringList aboveAnnotationText;
		QStringList belowAnnotationText;

		if (options.graphType == SCATTER)
		{
			QCPItemRect *rect = new QCPItemRect(customPlot);
			QPen rectPen("#0536b0");
			rectPen.setWidthF(1.3);
			rect->setPen(rectPen);
			rect->topLeft->setType(QCPItemPosition::ptAbsolute);
			rect->bottomRight->setType(QCPItemPosition::ptAbsolute);
			if (options.xAxisSpacingIndex == CONSTANT)
			{
				rect->topLeft->setCoords(customPlot->xAxis->coordToPixel(i) - 7, customPlot->yAxis->coordToPixel(velocityMax) - 7);
				rect->bottomRight->setCoords(customPlot->xAxis->coordToPixel(i) + 7, customPlot->yAxis->coordToPixel(velocityMin) + 7);
			}
			else
			{
				rect->topLeft->setCoords(customPlot->xAxis->coordToPixel(chargeWeight) - 7, customPlot->yAxis->coordToPixel(velocityMax) - 7);
				rect->bottomRight->setCoords(customPlot->xAxis->coordToPixel(chargeWeight) + 7, customPlot->yAxis->coordToPixel(velocityMin) + 7);
			}
		}

		if (options.showES && (series->muzzleVelocities.size() > 1))
		{
			QString annotation = QString("ES: %1").arg(es);
			if (options.esLocation == ABOVE_STRING)
			{
				aboveAnnotationText.append(annotation);
			}
			else
			{
				belowAnnotationText.append(annotation);
			}
		}

		if (options.showSD && (series->muzzleVelocities.size() > 1))
		{
			QString annotation = QString("SD: %1").arg(stdev, 0, 'f', 1);
			if (options.sdLocation == ABOVE_STRING)
			{
				aboveAnnotationText.append(annotation);
			}
			else
			{
				belowAnnotationText.append(annotation);
			}
		}

		if (options.showAvg)
		{
			QString annotation;
			if (series->muzzleVelocities.size() > 1)
			{
				annotation = QString("x\u0305: %1").arg(mean, 0, 'f', 1);
			}
			else
			{
				annotation = QString::number(mean);
			}

			if (options.avgLocation == ABOVE_STRING)
			{
				aboveAnnotationText.append(annotation);
			}
			else
			{
				belowAnnotationText.append(annotation);
			}
		}

		if (options.showVD)
		{
			if (prevMeanSet)
			{
				QString sign;
				int delta = round(mean - prevMean);
				if (delta < 0)
				{
					sign = QString("-");
				}
				else
				{
					sign = QString("+");
				}

				QString annotation = QString("%1%2").arg(sign).arg(abs(delta));
				if (options.vdLocation == ABOVE_STRING)
				{
					aboveAnnotationText.append(annotation);
				}
				else
				{
					belowAnnotationText.append(annotation);
				}
			}
		}

		// Obtain pixel coordinates for points
		double yCoordBelow;
		double yCoordAbove;

		if (options.graphType == SCATTER)
		{
			yCoordBelow = velocityMin;
			yCoordAbove = velocityMax;
		}
		else
		{
			if (qIsNaN(stdev))
			{
				yCoordBelow = mean;
				yCoordAbove = mean;
			}
			else
			{
				yCoordBelow = mean - stdev;
				yCoordAbove = mean + stdev;
			}
		}

		QCPItemText *belowAnnotation = new QCPItemText(customPlot);
		belowAnnotation->setText(belowAnnotationText.join('\n'));
		belowAnnotation->setFont(QFont("DejaVu Sans", scaleFontSize(9)));
		belowAnnotation->setColor(QColor("#4d4d4d"));
		belowAnnotation->position->setType(QCPItemPosition::ptAbsolute);
		if (options.xAxisSpacingIndex == CONSTANT)
		{
			belowAnnotation->position->setCoords(customPlot->xAxis->coordToPixel(i), customPlot->yAxis->coordToPixel(yCoordBelow) + 10);
		}
		else
		{
			belowAnnotation->position->setCoords(customPlot->xAxis->coordToPixel(chargeWeight), customPlot->yAxis->coordToPixel(yCoordBelow) + 10);
		}
		belowAnnotation->setPositionAlignment(Qt::AlignHCenter | Qt::AlignTop);
		belowAnnotation->setTextAlignment(Qt::AlignCenter);
		belowAnnotation->setBrush(QBrush(Qt::white));
		belowAnnotation->setClipToAxisRect(false);
		belowAnnotation->setLayer(customPlot->layer(5));

		QCPItemText *aboveAnnotation = new QCPItemText(customPlot);
		aboveAnnotation->setText(aboveAnnotationText.join('\n'));
		aboveAnnotation->setFont(QFont("DejaVu Sans", scaleFontSize(9)));
		aboveAnnotation->setColor(QColor("#4d4d4d"));
		aboveAnnotation->position->setType(QCPItemPosition::ptAbsolute);
		if (options.xAxisSpacingIndex == CONSTANT)
		{
			aboveAnnotation->position->setCoords(customPlot->xAxis->coordToPixel(i), customPlot->yAxis->coordToPixel(yCoordAbove) - 10);
		}
		else
		{
			aboveAnnotation->position->setCoords(customPlot->xAxis->coordToPixel(chargeWeight), customPlot->yAxis->coordToPixel(yCoordAbove) - 10);
		}
		aboveAnnotation->setPositionAlignment(Qt::AlignHCenter | Qt::AlignBottom);
		aboveAnnotation->setTextAlignment(Qt::AlignCenter);
		aboveAnnotation->setBrush(QBrush(Qt::white));
		aboveAnnotation->setClipToAxisRect(false);
		aboveAnnotation->setLayer(customPlot->layer(5));

		prevMean = mean;
		prevMeanSet = true;
	}
}

void GraphRenderer::saveGraphToFile(
	QWidget *parent,
	QCustomPlot *customPlot,
	const QString &graphTitle,
	const QString &prevSaveDir,
	QString *outSaveDir)
{
	QString fileName;
	if (graphTitle.isEmpty())
	{
		fileName = QString("graph.png");
	}
	else
	{
		fileName = QString(graphTitle).append(".png");
	}

	QString savePath = QDir(prevSaveDir).filePath(fileName);
	qDebug() << "graphTitle:" << graphTitle;
	qDebug() << "fileName:" << fileName;
	qDebug() << "savePath:" << savePath;

	QString path = QFileDialog::getSaveFileName(parent, "Save graph as image", savePath, 
		"PNG image (*.png);;JPG image (*.jpg);;PDF file (*.pdf)");
	qDebug() << "User selected save path:" << path;

	if (path.isEmpty())
	{
		qDebug() << "No path selected, bailing";
		return;
	}

	QFileInfo pathInfo(path);
	*outSaveDir = pathInfo.absolutePath();

	QStringList allowedExts;
	allowedExts << "png" << "jpg" << "pdf";

	QString pathExt = pathInfo.suffix().toLower();

	if (pathExt.isEmpty() || (!allowedExts.contains(pathExt)))
	{
		path.append(".png");
		pathExt = "png";
	}

	qDebug() << "Using save path:" << path;

	bool res;
	if (pathExt == "png")
	{
		res = customPlot->savePng(path, 1440, 625, 2.0);
	}
	else if (pathExt == "jpg")
	{
		res = customPlot->saveJpg(path, 1440, 625, 2.0);
	}
	else if (pathExt == "pdf")
	{
		res = customPlot->savePdf(path, 1440, 625);
	}
	else
	{
		qDebug() << "error, shouldn't be reached";
		res = false;
	}

	qDebug() << "save file res =" << res;

	if (res)
	{
		QMessageBox::information(parent, "Save file", QString("Saved file to '%1'").arg(path), QMessageBox::Ok, QMessageBox::Ok);
	}
	if (res == false)
	{
		QMessageBox::warning(parent, "Save file", QString("Unable to save file to '%1'\n\nPlease choose a different path").arg(path), QMessageBox::Ok, QMessageBox::Ok);
	}
}

QString GraphRenderer::getWeightUnit(int index)
{
	if (index == GRAINS)
	{
		return "gr";
	}
	else
	{
		return "g";
	}
}

QString GraphRenderer::getVelocityUnit(int index)
{
	if (index == FPS)
	{
		return "ft/s";
	}
	else
	{
		return "m/s";
	}
}

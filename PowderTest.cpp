#include <sstream>

#include "untar.h"
#include "miniz.h"
#include "ChronoPlotter.h"
#include "PowderTest.h"
#include "ChronographParsers.h"
#include "GraphRenderer.h"
#include "FileSelectionHandlers.h"
#include "SeriesDataManager.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

#include "xlsxdocument.h"
#include "xlsxchartsheet.h"
#include "xlsxcellrange.h"
#include "xlsxchart.h"
#include "xlsxrichstring.h"
#include "xlsxworkbook.h"

using namespace Powder;

PowderTest::PowderTest ( QWidget *parent )
	: QWidget(parent)
{
	qDebug() << "Powder test";

	graphPreview = NULL;
	prevLabRadarDir = QDir::homePath();
	prevMagnetoSpeedDir = QDir::homePath();
	prevProChronoDir = QDir::homePath();
	prevGarminDir = QDir::homePath();
	prevShotMarkerDir = QDir::homePath();
	prevSaveDir = QDir::homePath();

	/* Left panel */

	QVBoxLayout *leftLayout = new QVBoxLayout();

	QLabel *selectLabel = new QLabel("Select chronograph type\nto populate series data\n");
	selectLabel->setAlignment(Qt::AlignCenter);

	QPushButton *lrDirButton = new QPushButton("Select LabRadar directory");
	connect(lrDirButton, SIGNAL(clicked(bool)), this, SLOT(selectLabRadarDirectory(bool)));
	lrDirButton->setMinimumWidth(300);
	lrDirButton->setMaximumWidth(300);
	lrDirButton->setMinimumHeight(50);
	lrDirButton->setMaximumHeight(50);

	QPushButton *msFileButton = new QPushButton("Select MagnetoSpeed file");
	connect(msFileButton, SIGNAL(clicked(bool)), this, SLOT(selectMagnetoSpeedFile(bool)));
	msFileButton->setMinimumWidth(300);
	msFileButton->setMaximumWidth(300);
	msFileButton->setMinimumHeight(50);
	msFileButton->setMaximumHeight(50);

	QPushButton *pcFileButton = new QPushButton("Select ProChrono file");
	connect(pcFileButton, SIGNAL(clicked(bool)), this, SLOT(selectProChronoFile(bool)));
	pcFileButton->setMinimumWidth(300);
	pcFileButton->setMaximumWidth(300);
	pcFileButton->setMinimumHeight(50);
	pcFileButton->setMaximumHeight(50);

	QPushButton *gFileButton = new QPushButton("Select Garmin CSV/XLSX file");
	connect(gFileButton, SIGNAL(clicked(bool)), this, SLOT(selectGarminFile(bool)));
	gFileButton->setMinimumWidth(300);
	gFileButton->setMaximumWidth(300);
	gFileButton->setMinimumHeight(50);
	gFileButton->setMaximumHeight(50);

	QPushButton *smFileButton = new QPushButton("Select ShotMarker file");
	connect(smFileButton, SIGNAL(clicked(bool)), this, SLOT(selectShotMarkerFile(bool)));
	smFileButton->setMinimumWidth(300);
	smFileButton->setMaximumWidth(300);
	smFileButton->setMinimumHeight(50);
	smFileButton->setMaximumHeight(50);

	QPushButton *manualEntryButton = new QPushButton("Manual data entry");
	connect(manualEntryButton, SIGNAL(clicked(bool)), this, SLOT(manualDataEntry(bool)));
	manualEntryButton->setMinimumWidth(300);
	manualEntryButton->setMaximumWidth(300);
	manualEntryButton->setMinimumHeight(50);
	manualEntryButton->setMaximumHeight(50);

	QPushButton *prepareGarminButton = new QPushButton("Prepare Garmin files");
	connect(prepareGarminButton, SIGNAL(clicked(bool)), this, SLOT(prepareGarminFiles(bool)));
	prepareGarminButton->setMinimumWidth(300);
	prepareGarminButton->setMaximumWidth(300);
	prepareGarminButton->setMinimumHeight(50);
	prepareGarminButton->setMaximumHeight(50);

	QVBoxLayout *placeholderLayout = new QVBoxLayout();
	placeholderLayout->addStretch(0);
	placeholderLayout->addWidget(selectLabel);
	placeholderLayout->setAlignment(selectLabel, Qt::AlignCenter);
	placeholderLayout->addWidget(lrDirButton);
	placeholderLayout->setAlignment(lrDirButton, Qt::AlignCenter);
	placeholderLayout->addWidget(msFileButton);
	placeholderLayout->setAlignment(msFileButton, Qt::AlignCenter);
	placeholderLayout->addWidget(pcFileButton);
	placeholderLayout->setAlignment(pcFileButton, Qt::AlignCenter);
	placeholderLayout->addWidget(gFileButton);
	placeholderLayout->setAlignment(gFileButton, Qt::AlignCenter);
	placeholderLayout->addWidget(smFileButton);
	placeholderLayout->setAlignment(smFileButton, Qt::AlignCenter);
	placeholderLayout->addWidget(manualEntryButton);
	placeholderLayout->setAlignment(manualEntryButton, Qt::AlignCenter);
	placeholderLayout->addWidget(prepareGarminButton);
	placeholderLayout->setAlignment(prepareGarminButton, Qt::AlignCenter);
	placeholderLayout->addStretch(0);

	QWidget *placeholderWidget = new QWidget();
	placeholderWidget->setLayout(placeholderLayout);

	stackedWidget = new QStackedWidget();
	stackedWidget->addWidget(placeholderWidget);
	stackedWidget->setCurrentIndex(0);

	QVBoxLayout *stackedLayout = new QVBoxLayout();
	stackedLayout->addWidget(stackedWidget);
	QGroupBox *chronoGroupBox = new QGroupBox("Chronograph data:");
	chronoGroupBox->setLayout(stackedLayout);

	leftLayout->addWidget(chronoGroupBox);

	/* Right panel */

	QVBoxLayout *rightLayout = new QVBoxLayout();

	QFormLayout *detailsFormLayout = new QFormLayout();

	graphTitle = new QLineEdit();
	rifle = new QLineEdit();
	projectile = new QLineEdit();
	propellant = new QLineEdit();
	brass = new QLineEdit();
	primer = new QLineEdit();
	weather = new QLineEdit();
	detailsFormLayout->addRow(new QLabel("Graph title:"), graphTitle);
	detailsFormLayout->addRow(new QLabel("Rifle:"), rifle);
	detailsFormLayout->addRow(new QLabel("Projectile:"), projectile);
	detailsFormLayout->addRow(new QLabel("Propellant:"), propellant);
	detailsFormLayout->addRow(new QLabel("Brass:"), brass);
	detailsFormLayout->addRow(new QLabel("Primer:"), primer);
	detailsFormLayout->addRow(new QLabel("Weather:"), weather);

	QGroupBox *detailsGroupBox = new QGroupBox("Graph details:");
	detailsGroupBox->setLayout(detailsFormLayout);

	/* Graph options */

	QVBoxLayout *optionsLayout = new QVBoxLayout();

	QFormLayout *optionsFormLayout = new QFormLayout();

	graphType = new QComboBox();
	graphType->addItem("Scatter plot");
	graphType->addItem("Line chart + SD bars");
	optionsFormLayout->addRow(new QLabel("Graph type:"), graphType);

	weightUnits = new QComboBox();
	weightUnits->addItem("grain (gr)");
	weightUnits->addItem("gram (g)");
	optionsFormLayout->addRow(new QLabel("Weight units:"), weightUnits);

	velocityUnits = new QComboBox();
	velocityUnits->addItem("feet per second (ft/s)");
	velocityUnits->addItem("meters per second (m/s)");
	optionsFormLayout->addRow(new QLabel("Velocity units:"), velocityUnits);

	xAxisSpacing = new QComboBox();
	xAxisSpacing->addItem("Proportional");
	xAxisSpacing->addItem("Constant");
	connect(xAxisSpacing, SIGNAL(currentIndexChanged(int)), this, SLOT(xAxisSpacingChanged(int)));
	optionsFormLayout->addRow(new QLabel("X-axis spacing:"), xAxisSpacing);

	optionsLayout->addLayout(optionsFormLayout);
	optionsLayout->addWidget(new QHLine());

	QHBoxLayout *esLayout = new QHBoxLayout();
	esCheckBox = new QCheckBox();
	esCheckBox->setChecked(true);
	connect(esCheckBox, SIGNAL(clicked(bool)), this, SLOT(esCheckBoxChanged(bool)));
	esLayout->addWidget(esCheckBox, 0);
	esLabel = new QLabel("Show ES");
	esLayout->addWidget(esLabel, 1);
	esLocation = new QComboBox();
	esLocation->addItem("above shot strings");
	esLocation->addItem("below shot strings");
	esLayout->addWidget(esLocation);
	optionsLayout->addLayout(esLayout);

	QHBoxLayout *sdLayout = new QHBoxLayout();
	sdCheckBox = new QCheckBox();
	sdCheckBox->setChecked(true);
	connect(sdCheckBox, SIGNAL(clicked(bool)), this, SLOT(sdCheckBoxChanged(bool)));
	sdLayout->addWidget(sdCheckBox, 0);
	sdLabel = new QLabel("Show SD");
	sdLayout->addWidget(sdLabel, 1);
	sdLocation = new QComboBox();
	sdLocation->addItem("above shot strings");
	sdLocation->addItem("below shot strings");
	sdLayout->addWidget(sdLocation);
	optionsLayout->addLayout(sdLayout);

	QHBoxLayout *avgLayout = new QHBoxLayout();
	avgCheckBox = new QCheckBox();
	avgCheckBox->setChecked(false);
	connect(avgCheckBox, SIGNAL(clicked(bool)), this, SLOT(avgCheckBoxChanged(bool)));
	avgLayout->addWidget(avgCheckBox, 0);
	avgLabel = new QLabel("Show avg. velocity");
	avgLayout->addWidget(avgLabel, 1);
	avgLocation = new QComboBox();
	avgLocation->addItem("above shot strings");
	avgLocation->addItem("below shot strings");
	avgLocation->setEnabled(false);
	avgLayout->addWidget(avgLocation);
	optionsLayout->addLayout(avgLayout);

	QHBoxLayout *vdLayout = new QHBoxLayout();
	vdCheckBox = new QCheckBox();
	vdCheckBox->setChecked(true);
	connect(vdCheckBox, SIGNAL(clicked(bool)), this, SLOT(vdCheckBoxChanged(bool)));
	vdLayout->addWidget(vdCheckBox, 0);
	vdLabel = new QLabel("Show veloc. deltas");
	vdLayout->addWidget(vdLabel, 1);
	vdLocation = new QComboBox();
	vdLocation->addItem("above shot strings");
	vdLocation->addItem("below shot strings");
	vdLocation->setCurrentIndex(1);
	vdLayout->addWidget(vdLocation);
	optionsLayout->addLayout(vdLayout);

	QHBoxLayout *trendLayout = new QHBoxLayout();
	trendCheckBox = new QCheckBox();
	trendCheckBox->setChecked(false);
	connect(trendCheckBox, SIGNAL(clicked(bool)), this, SLOT(trendCheckBoxChanged(bool)));
	trendLayout->addWidget(trendCheckBox, 0);
	trendLabel = new QLabel("Show trend line");
	trendLayout->addWidget(trendLabel, 1);
	trendLineType = new QComboBox();
	trendLineType->addItem("solid line");
	trendLineType->addItem("dashed line");
	trendLineType->setCurrentIndex(1);
	trendLineType->setEnabled(false);
	trendLayout->addWidget(trendLineType);
	optionsLayout->addLayout(trendLayout);

	// Don't resize row heights if window height changes
	optionsLayout->addStretch(0);

	QGroupBox *optionsGroupBox = new QGroupBox("Graph options:");
	optionsGroupBox->setLayout(optionsLayout);

	QVBoxLayout *graphButtonsLayout = new QVBoxLayout();

	QPushButton *showGraphButton = new QPushButton("Show graph");
	connect(showGraphButton, SIGNAL(clicked(bool)), this, SLOT(showGraph(bool)));
	graphButtonsLayout->addWidget(showGraphButton);

	QPushButton *saveGraphButton = new QPushButton("Save graph as image");
	connect(saveGraphButton, SIGNAL(clicked(bool)), this, SLOT(saveGraph(bool)));
	graphButtonsLayout->addWidget(saveGraphButton);

	graphButtonsLayout->addStretch(0);

	/* Vertically position graph options and generate graph buttons */
	rightLayout->addWidget(detailsGroupBox);
	rightLayout->addWidget(optionsGroupBox);
	rightLayout->addLayout(graphButtonsLayout);

	/* Horizontally position series data and graph options */
	QHBoxLayout *pageLayout = new QHBoxLayout();
	pageLayout->addLayout(leftLayout, 2);
	pageLayout->addLayout(rightLayout, 0);

	this->setLayout(pageLayout);
}

void PowderTest::DisplaySeriesData ( void )
{
	SeriesDataManager::displaySeriesData(
		this,
		seriesData,
		stackedWidget,
		&scrollWidget,
		&scrollArea,
		&seriesGrid,
		&headerResult
	);
}

static bool ChronoSeriesComparator ( ChronoSeries *one, ChronoSeries *two )
{
	return (one->seriesNum < two->seriesNum);
}

void PowderTest::addNewClicked ( bool state )
{
	qDebug() << "addNewClicked state =" << state;

	// un-bold the button after the first click
	addNewButton->setStyleSheet("");

	int numRows = seriesGrid->rowCount();

	// Remove the stretch from the last row, we'll be using the row for our new series
	seriesGrid->setRowStretch(numRows - 1, 0);

	qDebug() << "numRows =" << numRows;

	ChronoSeries *series = new ChronoSeries();

	series->deleted = false;

	series->enabled = new QCheckBox();
	series->enabled->setChecked(true);

	connect(series->enabled, SIGNAL(stateChanged(int)), this, SLOT(seriesManualCheckBoxChanged(int)));
	seriesGrid->addWidget(series->enabled, numRows - 1, 0);

	int newSeriesNum = 1;
	for ( int i = seriesData.size() - 1; i >= 0; i-- )
	{
		ChronoSeries *series = seriesData.at(i);
		if ( ! series->deleted )
		{
			newSeriesNum = series->seriesNum + 1;
			qDebug() << "Found last un-deleted series" << series->seriesNum << "(" << series->name->text() << ") at index" << i;
			break;
		}
	}

	series->seriesNum = newSeriesNum;

	series->name = new QLabel(QString("Series %1").arg(newSeriesNum));
	seriesGrid->addWidget(series->name, numRows - 1, 1);

	series->chargeWeight = new QDoubleSpinBox();
	series->chargeWeight->setDecimals(2);
	series->chargeWeight->setSingleStep(0.1);
	series->chargeWeight->setMaximum(1000000);
	series->chargeWeight->setMinimumWidth(100);
	series->chargeWeight->setMaximumWidth(100);

	QHBoxLayout *chargeWeightLayout = new QHBoxLayout();
	chargeWeightLayout->addWidget(series->chargeWeight);
	chargeWeightLayout->addStretch(0);
	seriesGrid->addLayout(chargeWeightLayout, numRows - 1, 2);

	const char *velocityUnits2;
	if ( velocityUnits->currentIndex() == FPS )
	{
		velocityUnits2 = "ft/s";
	}
	else
	{
		velocityUnits2 = "m/s";
	}

	series->result = new QLabel(QString("0 shots, 0-0 %1").arg(velocityUnits2));
	seriesGrid->addWidget(series->result, numRows - 1, 3, Qt::AlignVCenter);

	series->enterDataButton = new QPushButton("Enter velocity data");
	connect(series->enterDataButton, SIGNAL(clicked(bool)), this, SLOT(enterDataClicked(bool)));
	series->enterDataButton->setFixedSize(series->enterDataButton->minimumSizeHint());
	seriesGrid->addWidget(series->enterDataButton, numRows - 1, 4, Qt::AlignLeft);

	series->deleteButton = new QPushButton();
	connect(series->deleteButton, SIGNAL(clicked(bool)), this, SLOT(deleteClicked(bool)));
	series->deleteButton->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
	series->deleteButton->setFixedSize(series->deleteButton->minimumSizeHint());
	seriesGrid->addWidget(series->deleteButton, numRows - 1, 5, Qt::AlignLeft);

	seriesData.append(series);

	// Add an empty stretch row at the end for proper spacing
	seriesGrid->setRowStretch(numRows, 1);
}

void PowderTest::enterDataClicked ( bool state )
{
	QPushButton *enterDataButton = qobject_cast<QPushButton *>(sender());

	qDebug() << "enterDataClicked state =" << state;

	// We have the button object, now locate its ChronoSeries object
	ChronoSeries *series = NULL;
	for ( int i = 0; i < seriesData.size(); i++ )
	{
		ChronoSeries *curSeries = seriesData.at(i);
		if ( enterDataButton == curSeries->enterDataButton )
		{
			series = curSeries;
			break;
		}
	}

	EnterVelocitiesDialog *dialog = new EnterVelocitiesDialog(series);
	int result = dialog->exec();

	qDebug() << "dialog result:" << result;

	if ( result )
	{
		qDebug() << "User OK'd dialog";

		QList<double> values = dialog->getValues();

		if ( values.size() == 0 )
		{
			qDebug() << "No valid velocities were provided, bailing...";
			return;
		}

		// We have the button object, now locate its row in the grid
		for ( int i = 0; i < seriesData.size(); i++ )
		{
			ChronoSeries *series = seriesData.at(i);
			if ( enterDataButton == series->enterDataButton )
			{
				qDebug() << "Setting velocities for Series" << series->seriesNum;

				series->muzzleVelocities = values;

				const char *velocityUnits2;
				if ( velocityUnits->currentIndex() == FPS )
				{
					velocityUnits2 = "ft/s";
				}
				else
				{
					velocityUnits2 = "m/s";
				}

				// Update the series result
				int totalShots = series->muzzleVelocities.size();
				double velocityMin = *std::min_element(series->muzzleVelocities.begin(), series->muzzleVelocities.end());
				double velocityMax = *std::max_element(series->muzzleVelocities.begin(), series->muzzleVelocities.end());
				series->result->setText(QString("%1 shot%2, %3-%4 %5").arg(totalShots).arg(totalShots > 1 ? "s" : "").arg(velocityMin).arg(velocityMax).arg(velocityUnits2));

				break;
			}
		}
	}
	else
	{
		qDebug() << "User cancelled dialog";
	}
}

void PowderTest::deleteClicked ( bool state )
{
	QPushButton *deleteButton = qobject_cast<QPushButton *>(sender());

	qDebug() << "deleteClicked state =" << state;

	int newSeriesNum = -1;

	// We have the checkbox object, now locate its row in the grid
	for ( int i = 0; i < seriesData.size(); i++ )
	{
		ChronoSeries *series = seriesData.at(i);
		if ( deleteButton == series->deleteButton )
		{
			qDebug() << "Series" << series->seriesNum << "(" << series->name->text() << ") was deleted";

			series->deleted = true;

			series->enabled->hide();
			series->name->hide();
			series->chargeWeight->hide();
			series->result->hide();
			series->enterDataButton->hide();
			series->deleteButton->hide();

			newSeriesNum = series->seriesNum;
		}
		else if ( (! series->deleted) && newSeriesNum > 0 )
		{
			qDebug() << "Updating Series" << series->seriesNum << "to Series" << newSeriesNum;

			series->seriesNum = newSeriesNum;
			series->name->setText(QString("Series %1").arg(newSeriesNum));

			newSeriesNum++;
		}
	}
}

void PowderTest::manualDataEntry ( bool state )
{
	qDebug() << "manualDataEntry state =" << state;

	// If we already have series data displayed, clear it out first. This call is a no-op if scrollWidget is not already added to stackedWidget.
	stackedWidget->removeWidget(scrollWidget);

	QVBoxLayout *scrollLayout = new QVBoxLayout();

	// Wrap grid in a widget to make the grid scrollable
	QWidget *scrollAreaWidget = new QWidget();

	seriesGrid = new QGridLayout(scrollAreaWidget);
	seriesGrid->setColumnStretch(0, 0);
	seriesGrid->setColumnStretch(1, 1);
	seriesGrid->setColumnStretch(2, 2);
	seriesGrid->setColumnStretch(3, 3);
	seriesGrid->setColumnStretch(4, 3);
	seriesGrid->setColumnStretch(5, 3);
	seriesGrid->setHorizontalSpacing(25);

	scrollArea = new QScrollArea();
	scrollArea->setWidget(scrollAreaWidget);
	scrollArea->setWidgetResizable(true);

	scrollLayout->addWidget(scrollArea);

	/* Create utilities toolbar under scroll area */

	addNewButton = new QPushButton("Add new series");
	addNewButton->setStyleSheet("font-weight: bold");
	connect(addNewButton, SIGNAL(clicked(bool)), this, SLOT(addNewClicked(bool)));
	addNewButton->setMinimumWidth(225);
	addNewButton->setMaximumWidth(225);

	QPushButton *loadNewButton = new QPushButton("Load new chronograph file");
	connect(loadNewButton, SIGNAL(clicked(bool)), this, SLOT(loadNewChronographData(bool)));
	loadNewButton->setMinimumWidth(225);
	loadNewButton->setMaximumWidth(225);

	QPushButton *autofillButton = new QPushButton("Auto-fill charge weights");
	connect(autofillButton, SIGNAL(clicked(bool)), this, SLOT(autofillClicked(bool)));
	autofillButton->setMinimumWidth(225);
	autofillButton->setMaximumWidth(225);

	QPushButton *rrButton = new QPushButton("Perform round robin");
	connect(rrButton, SIGNAL(clicked(bool)), this, SLOT(rrClicked(bool)));
	rrButton->setMinimumWidth(225);
	rrButton->setMaximumWidth(225);

	QHBoxLayout *utilitiesLayout = new QHBoxLayout();
	utilitiesLayout->addWidget(addNewButton);
	utilitiesLayout->addWidget(loadNewButton);
	utilitiesLayout->addWidget(autofillButton);
	utilitiesLayout->addWidget(rrButton);

	scrollLayout->addLayout(utilitiesLayout);

	scrollWidget = new QWidget();
	scrollWidget->setLayout(scrollLayout);

	stackedWidget->addWidget(scrollWidget);
	stackedWidget->setCurrentWidget(scrollWidget);

	QCheckBox *headerCheckBox = new QCheckBox();
	headerCheckBox->setChecked(true);
	connect(headerCheckBox, SIGNAL(stateChanged(int)), this, SLOT(headerCheckBoxChanged(int)));
	seriesGrid->addWidget(headerCheckBox, 0, 0);

	QLabel *headerName = new QLabel("Series Name");
	seriesGrid->addWidget(headerName, 0, 1, Qt::AlignVCenter);
	QLabel *headerChargeWeight = new QLabel("Charge Weight");
	seriesGrid->addWidget(headerChargeWeight, 0, 2, Qt::AlignVCenter);
	headerResult = new QLabel("Series Result");
	seriesGrid->addWidget(headerResult, 0, 3, Qt::AlignVCenter);
	QLabel *headerEnterData = new QLabel("");
	seriesGrid->addWidget(headerEnterData, 0, 4, Qt::AlignVCenter);
	QLabel *headerDelete = new QLabel("");
	seriesGrid->addWidget(headerDelete, 0, 5, Qt::AlignVCenter);

	// Only connect this signal for manual data entry
	connect(velocityUnits, SIGNAL(activated(int)), this, SLOT(velocityUnitsChanged(int)));

	/* Create initial row */

	ChronoSeries *series = new ChronoSeries();

	series->deleted = false;

	series->enabled = new QCheckBox();
	series->enabled->setChecked(true);

	connect(series->enabled, SIGNAL(stateChanged(int)), this, SLOT(seriesManualCheckBoxChanged(int)));
	seriesGrid->addWidget(series->enabled, 1, 0);

	series->seriesNum = 1;

	series->name = new QLabel("Series 1");
	seriesGrid->addWidget(series->name, 1, 1);

	series->chargeWeight = new QDoubleSpinBox();
	series->chargeWeight->setDecimals(2);
	series->chargeWeight->setSingleStep(0.1);
	series->chargeWeight->setMaximum(1000000);
	series->chargeWeight->setMinimumWidth(100);
	series->chargeWeight->setMaximumWidth(100);

	QHBoxLayout *chargeWeightLayout = new QHBoxLayout();
	chargeWeightLayout->addWidget(series->chargeWeight);
	chargeWeightLayout->addStretch(0);
	seriesGrid->addLayout(chargeWeightLayout, 1, 2);

	const char *velocityUnits2;
	if ( velocityUnits->currentIndex() == FPS )
	{
		velocityUnits2 = "ft/s";
	}
	else
	{
		velocityUnits2 = "m/s";
	}

	series->result = new QLabel(QString("0 shots, 0-0 %1").arg(velocityUnits2));
	seriesGrid->addWidget(series->result, 1, 3, Qt::AlignVCenter);

	series->enterDataButton = new QPushButton("Enter velocity data");
	connect(series->enterDataButton, SIGNAL(clicked(bool)), this, SLOT(enterDataClicked(bool)));
	series->enterDataButton->setFixedSize(series->enterDataButton->minimumSizeHint());
	seriesGrid->addWidget(series->enterDataButton, 1, 4, Qt::AlignLeft);

	series->deleteButton = new QPushButton();
	connect(series->deleteButton, SIGNAL(clicked(bool)), this, SLOT(deleteClicked(bool)));
	series->deleteButton->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
	series->deleteButton->setFixedSize(series->deleteButton->minimumSizeHint());
	seriesGrid->addWidget(series->deleteButton, 1, 5, Qt::AlignLeft);

	seriesGrid->setRowMinimumHeight(0, series->chargeWeight->sizeHint().height());

	seriesData.append(series);

	// Add an empty stretch row at the end for proper spacing
	seriesGrid->setRowStretch(seriesGrid->rowCount(), 1);
}

void PowderTest::showGraph ( bool state )
{
	qDebug() << "showGraph state =" << state;

	renderGraph(true);
}

void PowderTest::saveGraph ( bool state )
{
	qDebug() << "showGraph state =" << state;

	renderGraph(false);
}

void PowderTest::renderGraph ( bool displayGraphPreview )
{
	qDebug() << "renderGraph displayGraphPreview =" << displayGraphPreview;

	// Build graph options structure
	GraphOptions options;
	options.graphType = graphType->currentIndex();
	options.weightUnitsIndex = weightUnits->currentIndex();
	options.velocityUnitsIndex = velocityUnits->currentIndex();
	options.xAxisSpacingIndex = xAxisSpacing->currentIndex();
	options.graphTitle = graphTitle->text();
	options.rifle = rifle->text();
	options.projectile = projectile->text();
	options.propellant = propellant->text();
	options.brass = brass->text();
	options.primer = primer->text();
	options.weather = weather->text();
	options.showES = esCheckBox->isChecked();
	options.esLocation = esLocation->currentIndex();
	options.showSD = sdCheckBox->isChecked();
	options.sdLocation = sdLocation->currentIndex();
	options.showAvg = avgCheckBox->isChecked();
	options.avgLocation = avgLocation->currentIndex();
	options.showVD = vdCheckBox->isChecked();
	options.vdLocation = vdLocation->currentIndex();
	options.showTrend = trendCheckBox->isChecked();
	options.trendLineType = trendLineType->currentIndex();

	// Delegate to GraphRenderer
	GraphRenderer::renderGraph(
		this,
		seriesData,
		displayGraphPreview,
		options,
		prevSaveDir,
		&prevSaveDir,
		&graphPreview
	);
}

void PowderTest::seriesCheckBoxChanged ( int state )
{
	QCheckBox *checkBox = qobject_cast<QCheckBox *>(sender());

	qDebug() << "seriesCheckBoxChanged state =" << state << " checkBox =" << checkBox;

	// We have the checkbox object, now locate its row in the grid
	for ( int i = 0; i < seriesGrid->rowCount() - 1; i++ )
	{
		QWidget *rowCheckBox = seriesGrid->itemAtPosition(i, 0)->widget();

		if ( checkBox == rowCheckBox )
		{
			QWidget *seriesName = seriesGrid->itemAtPosition(i, 1)->widget();
			QWidget *chargeWeight = seriesGrid->itemAtPosition(i, 2)->layout()->itemAt(0)->widget();
			QWidget *seriesResult = seriesGrid->itemAtPosition(i, 3)->widget();
			QWidget *seriesDate = seriesGrid->itemAtPosition(i, 4)->widget();

			if ( checkBox->isChecked() )
			{
				qDebug() << "Grid row" << i << "was checked";

				seriesName->setEnabled(true);
				chargeWeight->setEnabled(true);
				seriesResult->setEnabled(true);
				seriesDate->setEnabled(true);
			}
			else
			{
				qDebug() << "Grid row" << i << "was unchecked";

				seriesName->setEnabled(false);
				chargeWeight->setEnabled(false);
				seriesResult->setEnabled(false);
				seriesDate->setEnabled(false);
			}

			return;
		}
	}
}

void PowderTest::seriesManualCheckBoxChanged ( int state )
{
	QCheckBox *checkBox = qobject_cast<QCheckBox *>(sender());

	qDebug() << "seriesManualCheckBoxChanged state =" << state << " checkBox =" << checkBox;

	// We have the checkbox object, now locate its row in the grid
	for ( int i = 0; i < seriesGrid->rowCount() - 1; i++ )
	{
		QWidget *rowCheckBox = seriesGrid->itemAtPosition(i, 0)->widget();

		if ( checkBox == rowCheckBox )
		{
			QWidget *seriesName = seriesGrid->itemAtPosition(i, 1)->widget();
			QWidget *chargeWeight = seriesGrid->itemAtPosition(i, 2)->layout()->itemAt(0)->widget();
			QWidget *seriesResult = seriesGrid->itemAtPosition(i, 3)->widget();
			QWidget *seriesEnterData = seriesGrid->itemAtPosition(i, 4)->widget();
			QWidget *seriesDelete = seriesGrid->itemAtPosition(i, 5)->widget();

			if ( checkBox->isChecked() )
			{
				qDebug() << "Grid row" << i << "was checked";

				seriesName->setEnabled(true);
				chargeWeight->setEnabled(true);
				seriesResult->setEnabled(true);
				seriesEnterData->setEnabled(true);
				seriesDelete->setEnabled(true);
			}
			else
			{
				qDebug() << "Grid row" << i << "was unchecked";

				seriesName->setEnabled(false);
				chargeWeight->setEnabled(false);
				seriesResult->setEnabled(false);
				seriesEnterData->setEnabled(false);
				seriesDelete->setEnabled(false);
			}

			return;
		}
	}
}

void PowderTest::headerCheckBoxChanged ( int state )
{
	qDebug() << "headerCheckBoxChanged state =" << state;

	if ( state == Qt::Checked )
	{
		qDebug() << "Header checkbox was checked";

		for ( int i = 0; i < seriesGrid->rowCount() - 1; i++ )
		{
			QCheckBox *checkBox = qobject_cast<QCheckBox *>(seriesGrid->itemAtPosition(i, 0)->widget());
			checkBox->setChecked(true);
		}
	}
	else
	{
		qDebug() << "Header checkbox was unchecked";

		for ( int i = 0; i < seriesGrid->rowCount() - 1; i++ )
		{
			QCheckBox *checkBox = qobject_cast<QCheckBox *>(seriesGrid->itemAtPosition(i, 0)->widget());
			checkBox->setChecked(false);
		}
	}
}

void PowderTest::velocityUnitsChanged ( int index )
{
	qDebug() << "velocityUnitsChanged index =" << index;

	/*
	 * This signal handler is only connected in manual data entry mode. When the velocity unit is changed, we
	 * need to iterate through and update every Series Result row to display the new unit.
	 */

	const char *velocityUnit;

	if ( index == FPS )
	{
		velocityUnit = "ft/s";
	}
	else
	{
		velocityUnit = "m/s";
	}

	for ( int i = 0; i < seriesData.size(); i++ )
	{
		ChronoSeries *series = seriesData.at(i);
		if ( ! series->deleted )
		{
			qDebug() << "Setting series" << i << "velocity unit to" << velocityUnit;

			if ( series->muzzleVelocities.size() == 0 )
			{
				series->result->setText(QString("0 shots, 0-0 %1").arg(velocityUnit));
			}
			else
			{
				int totalShots = series->muzzleVelocities.size();
				double velocityMin = *std::min_element(series->muzzleVelocities.begin(), series->muzzleVelocities.end());
				double velocityMax = *std::max_element(series->muzzleVelocities.begin(), series->muzzleVelocities.end());
				series->result->setText(QString("%1 shot%2, %3-%4 %5").arg(totalShots).arg(totalShots > 1 ? "s" : "").arg(velocityMin).arg(velocityMax).arg(velocityUnit));
			}
		}
	}
}

void PowderTest::esCheckBoxChanged ( bool state )
{
	qDebug() << "esCheckBoxChanged state =" << state;

	optionCheckBoxChanged(esCheckBox, esLabel, esLocation);
}

void PowderTest::sdCheckBoxChanged ( bool state )
{
	qDebug() << "sdsCheckBoxChanged state =" << state;

	optionCheckBoxChanged(sdCheckBox, sdLabel, sdLocation);
}

void PowderTest::avgCheckBoxChanged ( bool state )
{
	qDebug() << "avgCheckBoxChanged state =" << state;

	optionCheckBoxChanged(avgCheckBox, avgLabel, avgLocation);
}

void PowderTest::vdCheckBoxChanged ( bool state )
{
	qDebug() << "vdCheckBoxChanged state =" << state;

	optionCheckBoxChanged(vdCheckBox, vdLabel, vdLocation);
}

void PowderTest::trendCheckBoxChanged ( bool state )
{
	qDebug() << "trendCheckBoxChanged state =" << state;

	optionCheckBoxChanged(trendCheckBox, trendLabel, trendLineType);
}

void PowderTest::xAxisSpacingChanged ( int index )
{
	qDebug() << "xAxisSpacingChanged index =" << index;

	if ( index == CONSTANT )
	{
		trendCheckBox->setChecked(false);
		trendCheckBox->setEnabled(false);
		trendLabel->setStyleSheet("color: #878787");
		trendLineType->setEnabled(false);
	}
	else
	{
		trendCheckBox->setEnabled(true);
		trendLabel->setStyleSheet("");
		trendLineType->setEnabled(false); // we always re-enable the checkbox unchecked, so the combobox stays disabled
	}
}

void PowderTest::optionCheckBoxChanged ( QCheckBox *checkBox, QLabel *label, QComboBox *comboBox )
{
	if ( checkBox->isChecked() )
	{
		qDebug() << "checkbox was checked";
		comboBox->setEnabled(true);
	}
	else
	{
		qDebug() << "checkbox was unchecked";
		comboBox->setEnabled(false);
	}
}

void PowderTest::loadNewChronographData ( bool state )
{
	qDebug() << "loadNewChronographData state =" << state;

	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(this, "Load new data", "Are you sure you want to load new chronograph data?\n\nThis will clear your current work.", QMessageBox::Yes | QMessageBox::Cancel);

	if ( reply == QMessageBox::Yes )
	{
		qDebug() << "User said yes";

		// Hide the chronograph data screen. This returns to the initial screen to choose a new chronograph file.
		stackedWidget->removeWidget(scrollWidget);

		// Delete the loaded chronograph data
		seriesData.clear();

		// Disconnect the velocity units header signal (used in manual data entry), if necessary
		disconnect(velocityUnits, SIGNAL(activated(int)), this, SLOT(velocityUnitsChanged(int)));
	}
	else
	{
		qDebug() << "User said cancel";
	}
}

void PowderTest::selectLabRadarDirectory ( bool state )
{
	qDebug() << "selectLabRadarDirectory state =" << state;

	QList<ChronoSeries*> newSeriesData = FileSelectionHandlers::selectLabRadarDirectory(
		this,
		prevLabRadarDir,
		&prevLabRadarDir
	);

	if (!newSeriesData.empty())
	{
		seriesData = newSeriesData;
		DisplaySeriesData();
	}
}

void PowderTest::selectMagnetoSpeedFile ( bool state )
{
	qDebug() << "selectMagnetoSpeedFile state =" << state;

	QList<ChronoSeries*> newSeriesData = FileSelectionHandlers::selectMagnetoSpeedFile(
		this,
		prevMagnetoSpeedDir,
		&prevMagnetoSpeedDir
	);

	if (!newSeriesData.empty())
	{
		seriesData = newSeriesData;
		DisplaySeriesData();
	}
}

void PowderTest::selectProChronoFile ( bool state )
{
	qDebug() << "selectProChronoFile state =" << state;

	QList<ChronoSeries*> newSeriesData = FileSelectionHandlers::selectProChronoFile(
		this,
		prevProChronoDir,
		&prevProChronoDir
	);

	if (!newSeriesData.empty())
	{
		seriesData = newSeriesData;
		DisplaySeriesData();
	}
}

void PowderTest::selectGarminFile ( bool state )
{
	qDebug() << "selectGarminFile state =" << state;

	QList<ChronoSeries*> newSeriesData = FileSelectionHandlers::selectGarminFile(
		this,
		prevGarminDir,
		&prevGarminDir
	);

	if (!newSeriesData.empty())
	{
		seriesData = newSeriesData;
		DisplaySeriesData();
	}
}

void PowderTest::selectShotMarkerFile ( bool state )
{
	qDebug() << "selectShotMarkerFile state =" << state;

	QList<ChronoSeries*> newSeriesData = FileSelectionHandlers::selectShotMarkerFile(
		this,
		prevShotMarkerDir,
		&prevShotMarkerDir
	);

	if (!newSeriesData.empty())
	{
		seriesData = newSeriesData;
		DisplaySeriesData();
	}
}

void PowderTest::rrClicked ( bool state )
{
	qDebug() << "rrClicked state =" << state;

	RoundRobinDialog *dialog = new RoundRobinDialog(this);
	int result = dialog->exec();

	qDebug() << "dialog result:" << result;

	if ( result )
	{
		qDebug() << "Performing series conversion";

		QList<QList<double> > enabledSeriesVelocs;
		for ( int i = 0; i < seriesData.size(); i++ )
		{
			ChronoSeries *series = seriesData.at(i);
			if ( series->enabled->isChecked() )
			{
				enabledSeriesVelocs.append(series->muzzleVelocities);
			}
		}

		// Iterate the number of velocities in the enabled series
		QList<ChronoSeries *> newSeriesData;
		for ( int i = 0; i < enabledSeriesVelocs.at(0).size(); i++ )
		{
			// Grab the Xth velocity in each of the enabled series and create a new series with them
			QList<double> newVelocs;
			for ( int j = 0; j < enabledSeriesVelocs.size(); j++ )
			{
				newVelocs.append(enabledSeriesVelocs.at(j).at(i));
			}

			ChronoSeries *newSeries = new ChronoSeries();

			newSeries->seriesNum = i;

			newSeries->name = new QLabel(QString("Series %1").arg(i + 1));

			newSeries->muzzleVelocities = newVelocs;

			newSeries->enabled = new QCheckBox();
			newSeries->enabled->setChecked(true);

			newSeries->chargeWeight = new QDoubleSpinBox();
			newSeries->chargeWeight->setDecimals(2);
			newSeries->chargeWeight->setSingleStep(0.1);
			newSeries->chargeWeight->setMaximum(1000000);
			newSeries->chargeWeight->setMinimumWidth(100);
			newSeries->chargeWeight->setMaximumWidth(100);

			newSeriesData.append(newSeries);
		}

		// Replace the current series data with the new one
		seriesData = newSeriesData;

		// Proceed to display the data
		DisplaySeriesData();
	}
	else
	{
		qDebug() << "Not performing series conversion";
	}
}

void PowderTest::autofillClicked ( bool state )
{
	qDebug() << "autofillClicked state =" << state;

	AutofillDialog *dialog = new AutofillDialog(this);
	int result = dialog->exec();

	qDebug() << "dialog result:" << result;

	if ( result )
	{
		qDebug() << "User OK'd dialog";

		AutofillValues *values = dialog->getValues();

		double currentCharge = values->startingCharge;

		for ( int i = 0; i < seriesData.size(); i++ )
		{
			ChronoSeries *series = seriesData.at(i);
			if ( (! series->deleted) && series->enabled->isChecked() )
			{
				qDebug() << "Setting series" << i << "to" << currentCharge;
				series->chargeWeight->setValue(currentCharge);
				if ( values->increasing )
				{
					currentCharge += values->interval;
				}
				else
				{
					currentCharge -= values->interval;
				}
			}
		}
	}
	else
	{
		qDebug() << "User cancelled dialog";
	}
}

void PowderTest::prepareGarminFiles ( bool state )
{
	qDebug() << "prepareGarminFiles state =" << state;

	// Open file selection dialog to select 2 or more CSV files
	QStringList csvFiles = QFileDialog::getOpenFileNames(
		this,
		"Select Garmin CSV files (2 or more)",
		prevGarminDir,
		"CSV files (*.csv)"
	);

	if (csvFiles.isEmpty())
	{
		qDebug() << "User didn't select any files, bail";
		return;
	}

	if (csvFiles.size() < 2)
	{
		qDebug() << "User selected less than 2 files";
		QMessageBox *msg = new QMessageBox();
		msg->setIcon(QMessageBox::Warning);
		msg->setText("Please select at least 2 CSV files to combine.");
		msg->setWindowTitle("Not Enough Files");
		msg->exec();
		return;
	}

	qDebug() << "Selected" << csvFiles.size() << "CSV files";

	// Update the previous directory
	QFileInfo firstFileInfo(csvFiles.at(0));
	prevGarminDir = firstFileInfo.absolutePath();

	// Create a new XLSX document
	QXlsx::Document xlsx;

	// Process each CSV file and add it as a worksheet
	for (int i = 0; i < csvFiles.size(); i++)
	{
		QString csvPath = csvFiles.at(i);
		qDebug() << "Processing CSV file:" << csvPath;

		QFile csvFile(csvPath);
		if (!csvFile.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			qDebug() << "Failed to open CSV file:" << csvPath;
			QMessageBox *msg = new QMessageBox();
			msg->setIcon(QMessageBox::Critical);
			msg->setText(QString("Failed to open file:\n%1").arg(csvPath));
			msg->setWindowTitle("Error");
			msg->exec();
			return;
		}

		QTextStream csv(&csvFile);

		// Read the CSV content
		QList<QStringList> rows;
		while (!csv.atEnd())
		{
			QString line = csv.readLine();
			QStringList cols = line.split(",");

			// Trim whitespace from cells
			QMutableStringListIterator it(cols);
			while (it.hasNext())
			{
				it.next();
				it.setValue(it.value().trimmed());
			}

			rows.append(cols);
		}

		csvFile.close();

		if (rows.isEmpty())
		{
			qDebug() << "CSV file is empty:" << csvPath;
			continue;
		}

		// Create worksheet name
		QString sheetName = QString("Sheet%1").arg(i + 1);
		
		qDebug() << "Creating worksheet:" << sheetName;

		// For the first sheet, rename the default sheet instead of adding a new one
		if (i == 0)
		{
			xlsx.renameSheet("Sheet1", sheetName);
			xlsx.selectSheet(sheetName);
		}
		else
		{
			xlsx.addSheet(sheetName);
			xlsx.selectSheet(sheetName);
		}

		// Write the CSV data to the worksheet
		for (int row = 0; row < rows.size(); row++)
		{
			QStringList cols = rows.at(row);
			for (int col = 0; col < cols.size(); col++)
			{
				// Write to Excel (1-based indexing)
				xlsx.write(row + 1, col + 1, cols.at(col));
			}
		}
		
		qDebug() << "Wrote" << rows.size() << "rows to worksheet" << sheetName;
	}

	// Present the user with a save dialog
	QString savePath = QFileDialog::getSaveFileName(
		this,
		"Save combined Garmin XLSX file",
		prevGarminDir,
		"Excel files (*.xlsx)"
	);

	if (savePath.isEmpty())
	{
		qDebug() << "User cancelled save dialog";
		return;
	}

	// Ensure the file has .xlsx extension
	if (!savePath.endsWith(".xlsx", Qt::CaseInsensitive))
	{
		savePath += ".xlsx";
	}

	qDebug() << "Saving XLSX file to:" << savePath;

	// Save the XLSX file
	bool saveSuccess = xlsx.saveAs(savePath);

	if (saveSuccess)
	{
		qDebug() << "XLSX file saved successfully";
		QMessageBox *msg = new QMessageBox();
		msg->setIcon(QMessageBox::Information);
		msg->setText(QString("Successfully created multi-series Garmin file!\n\nSaved to:\n%1").arg(savePath));
		msg->setWindowTitle("Success");
		msg->exec();
	}
	else
	{
		qDebug() << "Failed to save XLSX file";
		QMessageBox *msg = new QMessageBox();
		msg->setIcon(QMessageBox::Critical);
		msg->setText(QString("Failed to save file:\n%1").arg(savePath));
		msg->setWindowTitle("Error");
		msg->exec();
	}
}

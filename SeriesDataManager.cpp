#include "SeriesDataManager.h"
#include "PowderTest.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QDebug>
#include <algorithm>

using namespace Powder;

static bool ChronoSeriesComparator(ChronoSeries *one, ChronoSeries *two)
{
	return (one->seriesNum < two->seriesNum);
}

void SeriesDataManager::displaySeriesData(
	QWidget *parent,
	QList<ChronoSeries*> &seriesData,
	QStackedWidget *stackedWidget,
	QWidget **outScrollWidget,
	QScrollArea **outScrollArea,
	QGridLayout **outSeriesGrid,
	QLabel **outHeaderResult)
{
	// Sort the list by series number
	std::sort(seriesData.begin(), seriesData.end(), ChronoSeriesComparator);

	// If we already have series data displayed, clear it out first
	stackedWidget->removeWidget(*outScrollWidget);

	QVBoxLayout *scrollLayout = new QVBoxLayout();

	// Wrap grid in a widget to make the grid scrollable
	QWidget *scrollAreaWidget = new QWidget();

	*outSeriesGrid = new QGridLayout(scrollAreaWidget);
	(*outSeriesGrid)->setColumnStretch(0, 0);
	(*outSeriesGrid)->setColumnStretch(1, 1);
	(*outSeriesGrid)->setColumnStretch(2, 2);
	(*outSeriesGrid)->setColumnStretch(3, 3);
	(*outSeriesGrid)->setColumnStretch(4, 3);
	(*outSeriesGrid)->setHorizontalSpacing(25);

	*outScrollArea = new QScrollArea();
	(*outScrollArea)->setWidget(scrollAreaWidget);
	(*outScrollArea)->setWidgetResizable(true);

	scrollLayout->addWidget(*outScrollArea);

	/* Create utilities toolbar under scroll area */
	QPushButton *loadNewButton = new QPushButton("Load new chronograph file");
	QObject::connect(loadNewButton, SIGNAL(clicked(bool)), parent, SLOT(loadNewChronographData(bool)));
	loadNewButton->setMinimumWidth(225);
	loadNewButton->setMaximumWidth(225);

	QPushButton *rrButton = new QPushButton("Convert from round-robin");
	QObject::connect(rrButton, SIGNAL(clicked(bool)), parent, SLOT(rrClicked(bool)));
	rrButton->setMinimumWidth(225);
	rrButton->setMaximumWidth(225);

	QPushButton *autofillButton = new QPushButton("Auto-fill charge weights");
	QObject::connect(autofillButton, SIGNAL(clicked(bool)), parent, SLOT(autofillClicked(bool)));
	autofillButton->setMinimumWidth(225);
	autofillButton->setMaximumWidth(225);

	QHBoxLayout *utilitiesLayout = new QHBoxLayout();
	utilitiesLayout->addWidget(loadNewButton);
	utilitiesLayout->addWidget(rrButton);
	utilitiesLayout->addWidget(autofillButton);

	scrollLayout->addLayout(utilitiesLayout);

	*outScrollWidget = new QWidget();
	(*outScrollWidget)->setLayout(scrollLayout);

	stackedWidget->addWidget(*outScrollWidget);
	stackedWidget->setCurrentWidget(*outScrollWidget);

	QCheckBox *headerCheckBox = new QCheckBox();
	headerCheckBox->setChecked(true);
	QObject::connect(headerCheckBox, SIGNAL(stateChanged(int)), parent, SLOT(headerCheckBoxChanged(int)));
	(*outSeriesGrid)->addWidget(headerCheckBox, 0, 0);

	/* Headers for series data */
	QLabel *headerName = new QLabel("Series Name");
	(*outSeriesGrid)->addWidget(headerName, 0, 1, Qt::AlignVCenter);
	QLabel *headerChargeWeight = new QLabel("Charge Weight");
	(*outSeriesGrid)->addWidget(headerChargeWeight, 0, 2, Qt::AlignVCenter);
	*outHeaderResult = new QLabel("Series Result");
	(*outSeriesGrid)->addWidget(*outHeaderResult, 0, 3, Qt::AlignVCenter);
	QLabel *headerDate = new QLabel("Series Date");
	(*outSeriesGrid)->addWidget(headerDate, 0, 4, Qt::AlignVCenter);

	for (int i = 0; i < seriesData.size(); i++)
	{
		ChronoSeries *series = seriesData.at(i);

		QObject::connect(series->enabled, SIGNAL(stateChanged(int)), parent, SLOT(seriesCheckBoxChanged(int)));

		(*outSeriesGrid)->addWidget(series->enabled, i + 1, 0);
		(*outSeriesGrid)->addWidget(series->name, i + 1, 1, Qt::AlignVCenter);

		QHBoxLayout *chargeWeightLayout = new QHBoxLayout();
		chargeWeightLayout->addWidget(series->chargeWeight);
		chargeWeightLayout->addStretch(0);
		(*outSeriesGrid)->addLayout(chargeWeightLayout, i + 1, 2);

		int totalShots = series->muzzleVelocities.size();
		double velocityMin = *std::min_element(series->muzzleVelocities.begin(), series->muzzleVelocities.end());
		double velocityMax = *std::max_element(series->muzzleVelocities.begin(), series->muzzleVelocities.end());
		QLabel *resultLabel = new QLabel(QString("%1 shot%2, %3-%4 %5").arg(totalShots).arg(totalShots > 1 ? "s" : "").arg(velocityMin).arg(velocityMax).arg(series->velocityUnits));
		(*outSeriesGrid)->addWidget(resultLabel, i + 1, 3, Qt::AlignVCenter);

		QLabel *datetimeLabel = new QLabel(QString("%1 %2").arg(series->firstDate).arg(series->firstTime));
		(*outSeriesGrid)->addWidget(datetimeLabel, i + 1, 4, Qt::AlignVCenter);

		(*outSeriesGrid)->setRowMinimumHeight(0, series->chargeWeight->sizeHint().height());
	}

	// Add an empty stretch row at the end for proper spacing
	(*outSeriesGrid)->setRowStretch(seriesData.size() + 1, 1);

	(*outScrollArea)->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	(*outScrollArea)->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	(*outScrollArea)->setWidgetResizable(true);
}

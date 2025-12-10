#ifndef SERIES_DATA_MANAGER_H
#define SERIES_DATA_MANAGER_H

#include <QWidget>
#include <QList>

class QStackedWidget;
class QScrollArea;
class QGridLayout;
class QLabel;

namespace Powder
{
	struct ChronoSeries;

	class SeriesDataManager
	{
	public:
		static void displaySeriesData(
			QWidget *parent,
			QList<ChronoSeries*> &seriesData,
			QStackedWidget *stackedWidget,
			QWidget **outScrollWidget,
			QScrollArea **outScrollArea,
			QGridLayout **outSeriesGrid,
			QLabel **outHeaderResult
		);
	};
}

#endif // SERIES_DATA_MANAGER_H

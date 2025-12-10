#ifndef GRAPH_RENDERER_H
#define GRAPH_RENDERER_H

#include <QWidget>
#include <QList>
#include <QString>

class QCustomPlot;
class GraphPreview;

namespace Powder
{
	struct ChronoSeries;

	struct GraphOptions
	{
		int graphType;
		int weightUnitsIndex;
		int velocityUnitsIndex;
		int xAxisSpacingIndex;
		QString graphTitle;
		QString rifle;
		QString projectile;
		QString propellant;
		QString brass;
		QString primer;
		QString weather;
		bool showES;
		int esLocation;
		bool showSD;
		int sdLocation;
		bool showAvg;
		int avgLocation;
		bool showVD;
		int vdLocation;
		bool showTrend;
		int trendLineType;
	};

	class GraphRenderer
	{
	public:
		static void renderGraph(
			QWidget *parent,
			const QList<ChronoSeries*> &seriesData,
			bool displayGraphPreview,
			const GraphOptions &options,
			const QString &prevSaveDir,
			QString *outSaveDir,
			GraphPreview **outGraphPreview
		);

	private:
		static bool validateSeries(
			QWidget *parent,
			const QList<ChronoSeries*> &seriesData,
			int *outNumEnabled
		);

		static QList<ChronoSeries*> getEnabledSeries(
			const QList<ChronoSeries*> &seriesData
		);

		static bool checkDuplicateChargeWeights(
			QWidget *parent,
			const QList<ChronoSeries*> &seriesToGraph,
			int xAxisSpacingIndex,
			int *outXAxisSpacing
		);

		static void configureGraph(
			QCustomPlot *customPlot,
			const GraphOptions &options
		);

		static void addAnnotations(
			QCustomPlot *customPlot,
			const QList<ChronoSeries*> &seriesToGraph,
			const GraphOptions &options
		);

		static void saveGraphToFile(
			QWidget *parent,
			QCustomPlot *customPlot,
			const QString &graphTitle,
			const QString &prevSaveDir,
			QString *outSaveDir
		);

		static QString getWeightUnit(int index);
		static QString getVelocityUnit(int index);
	};
}

#endif // GRAPH_RENDERER_H

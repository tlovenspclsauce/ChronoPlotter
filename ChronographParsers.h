#ifndef CHRONOGRAPH_PARSERS_H
#define CHRONOGRAPH_PARSERS_H

#include <QTextStream>
#include <QList>
#include <QString>
#include "xlsxdocument.h"

namespace Powder
{
	struct ChronoSeries;

	class ChronographParsers
	{
	public:
		// LabRadar parser
		static ChronoSeries* extractLabRadarSeries(QTextStream &csv);
		
		// MagnetoSpeed parser
		static QList<ChronoSeries*> extractMagnetoSpeedSeries(QTextStream &csv);
		
		// ProChrono parsers
		static QList<ChronoSeries*> extractProChronoSeries(QTextStream &csv);
		static QList<ChronoSeries*> extractProChronoSeries_format2(QTextStream &csv);
		
		// Garmin parsers
		static QList<ChronoSeries*> extractGarminSeries_xlsx(QXlsx::Document &xlsx);
		static QList<ChronoSeries*> extractGarminSeries_csv(QTextStream &csv);
		
		// ShotMarker parser
		static QList<ChronoSeries*> extractShotMarkerSeriesTar(QString path);
	
	private:
		// CSV parsing helper
		static bool readCSVRow(QTextStream &in, QStringList *row);
	};
}

#endif // CHRONOGRAPH_PARSERS_H

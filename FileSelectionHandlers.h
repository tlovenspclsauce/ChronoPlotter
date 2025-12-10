#ifndef FILE_SELECTION_HANDLERS_H
#define FILE_SELECTION_HANDLERS_H

#include <QString>
#include <QList>
#include <QWidget>

namespace Powder
{
	struct ChronoSeries;

	class FileSelectionHandlers
	{
	public:
		static QList<ChronoSeries*> selectLabRadarDirectory(
			QWidget *parent,
			const QString &prevDir,
			QString *outDir
		);

		static QList<ChronoSeries*> selectMagnetoSpeedFile(
			QWidget *parent,
			const QString &prevDir,
			QString *outDir
		);

		static QList<ChronoSeries*> selectProChronoFile(
			QWidget *parent,
			const QString &prevDir,
			QString *outDir
		);

		static QList<ChronoSeries*> selectGarminFile(
			QWidget *parent,
			const QString &prevDir,
			QString *outDir
		);

		static QList<ChronoSeries*> selectShotMarkerFile(
			QWidget *parent,
			const QString &prevDir,
			QString *outDir
		);
	};
}

#endif // FILE_SELECTION_HANDLERS_H

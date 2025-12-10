#include "PowderTest.h"
#include "ChronoPlotter.h"
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QDebug>
#include <algorithm>

using namespace Powder;

RoundRobinDialog::RoundRobinDialog(PowderTest *main, QDialog *parent)
	: QDialog(parent)
{
	qDebug() << "Round-robin dialog";

	setWindowTitle("Convert from round-robin");

	QLabel *label = new QLabel();
	label->setTextFormat(Qt::RichText);
	label->setText("<p>This feature handles chronograph data recorded using the \"round-robin\" method popular with <a href=\"http://www.ocwreloading.com/\">OCW testing</a>.<p>For example a shooter might record three chronograph series, where each series contains 10 shots with 10 different charge weights. Use this feature to \"convert\" the data back into 10 series of three-shot strings.<p>Data recorded using the <a href=\"http://www.65guys.com/10-round-load-development-ladder-test/\">Satterlee method</a> can be converted by using this feature with only a single series enabled (rather than multiple).<p>Note: This will <i>not</i> alter your CSV files, this only converts the data loaded in ChronoPlotter. If converting multiple series, it's assumed that charge weights are shot in the same order in each series.<br>");
	label->setOpenExternalLinks(true);
	label->setWordWrap(true);

	QVBoxLayout *layout = new QVBoxLayout();
	layout->addWidget(label);
	layout->addWidget(new QHLine());

	QList<QList<double> > seriesVelocs;
	for (int i = 0; i < main->seriesData.size(); i++)
	{
		ChronoSeries *series = main->seriesData.at(i);
		if (series->enabled->isChecked())
		{
			seriesVelocs.append(series->muzzleVelocities);
		}
	}

	qDebug() << "Number of enabled series:" << seriesVelocs.size();
	qDebug() << "seriesVelocs:" << seriesVelocs;

	QList<int> numVelocs;
	for (int i = 0; i < seriesVelocs.size(); i++)
	{
		numVelocs.append(seriesVelocs.at(i).size());
	}

	qDebug() << "numVelocs:" << numVelocs;

	bool equalLens = std::equal(numVelocs.begin() + 1, numVelocs.end(), numVelocs.begin());

	qDebug() << "equalLens:" << equalLens;

	QLabel *detected = new QLabel();
	QDialogButtonBox *buttonBox;

	if (equalLens)
	{
		detected->setText(QString("<center><br>Detected <b>%1</b> enabled series of <b>%2</b> shots each.<p>Click <b>OK</b> to convert this data into <b>%3</b> series of <b>%4</b> shots each.<br>").arg(seriesVelocs.size()).arg(numVelocs.at(0)).arg(numVelocs.at(0)).arg(seriesVelocs.size()));

		buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		connect(buttonBox, &QDialogButtonBox::accepted, this, &RoundRobinDialog::accept);
		connect(buttonBox, &QDialogButtonBox::rejected, this, &RoundRobinDialog::reject);

	}
	else
	{
		detected->setText(QString("<center><br>Detected <b>%1</b> enabled series of <b>different</b> lengths.<p>Series with the same number of shots are required to convert.<br>").arg(seriesVelocs.size()));

		buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
		connect(buttonBox, &QDialogButtonBox::rejected, this, &RoundRobinDialog::reject);
	}

	detected->setTextFormat(Qt::RichText);
	detected->setWordWrap(true);
	layout->addWidget(detected);
	layout->addWidget(buttonBox);
	setLayout(layout);

	setFixedSize(sizeHint());
}

#include "PowderTest.h"
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QDebug>

using namespace Powder;

EnterVelocitiesDialog::EnterVelocitiesDialog(ChronoSeries *series, QDialog *parent)
	: QDialog(parent)
{
	qDebug() << "Enter velocities dialog";

	setWindowTitle("Enter velocities");

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &EnterVelocitiesDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &EnterVelocitiesDialog::reject);

	QVBoxLayout *layout = new QVBoxLayout();

	velocitiesEntered = new QLabel(QString("Velocities entered: %1").arg(series->muzzleVelocities.size()));
	layout->addWidget(velocitiesEntered);

	textEdit = new QTextEdit();
	textEdit->setPlaceholderText("Enter velocity numbers here, each one on a new line.\n\nFor example:\n2785\n2782\n2798");

	if (series->muzzleVelocities.size() > 0)
	{
		QString prevVelocs;
		for (int i = 0; i < series->muzzleVelocities.size(); i++)
		{
			prevVelocs += QString::number(series->muzzleVelocities.at(i));
			if (i < (series->muzzleVelocities.size() - 1))
			{
				prevVelocs += "\n";
			}
		}

		textEdit->setPlainText(prevVelocs);
	}

	connect(textEdit, SIGNAL(textChanged()), this, SLOT(textChanged()));
	layout->addWidget(textEdit);

	layout->addWidget(buttonBox);
	setLayout(layout);

	setFixedSize(sizeHint());
}

void EnterVelocitiesDialog::textChanged()
{
	QStringList list = textEdit->toPlainText().split("\n");

	int numVelocities = 0;
	for (int i = 0; i < list.length(); i++)
	{
		// validate inputted number
		bool ok;
		list.at(i).toInt(&ok);
		if (ok)
		{
			numVelocities++;
		}
		else
		{
			qDebug() << "Skipping invalid number:" << list.at(i);
		}
	}

	velocitiesEntered->setText(QString("Velocities entered: %1").arg(numVelocities));
}

QList<double> EnterVelocitiesDialog::getValues(void)
{
	QList<double> values;

	QStringList list = textEdit->toPlainText().split("\n");
	qDebug() << list;

	for (int i = 0; i < list.length(); i++)
	{
		bool ok;
		int velocity = list.at(i).toInt(&ok);
		if (ok)
		{
			values.append(velocity);
		}
		else
		{
			qDebug() << "Skipping invalid number:" << list.at(i);
		}
	}

	return values;
}

#include "PowderTest.h"
#include "ChronoPlotter.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>

using namespace Powder;

AutofillDialog::AutofillDialog(PowderTest *main, QDialog *parent)
	: QDialog(parent)
{
	qDebug() << "Autofill dialog";

	setWindowTitle("Auto-fill charge weights");

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &AutofillDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &AutofillDialog::reject);

	QLabel *weightUnitsLabel;
	if (main->weightUnits->currentIndex() == GRAINS)
	{
		weightUnitsLabel = new QLabel("gr");
	}
	else
	{
		weightUnitsLabel = new QLabel("g");
	}

	QFormLayout *formLayout = new QFormLayout();

	startingCharge = new QDoubleSpinBox();
	startingCharge->setDecimals(2);
	startingCharge->setSingleStep(0.1);
	startingCharge->setMaximum(1000000);
	startingCharge->setMinimumWidth(100);
	startingCharge->setMaximumWidth(100);

	QHBoxLayout *startingChargeLayout = new QHBoxLayout();
	startingChargeLayout->addWidget(startingCharge);
	startingChargeLayout->addWidget(weightUnitsLabel);

	formLayout->addRow(new QLabel("Starting value:"), startingChargeLayout);

	interval = new QDoubleSpinBox();
	interval->setDecimals(2);
	interval->setSingleStep(0.1);
	interval->setMaximum(1000000);
	interval->setMinimumWidth(100);
	interval->setMaximumWidth(100);

	QHBoxLayout *intervalLayout = new QHBoxLayout();
	intervalLayout->addWidget(interval);
	intervalLayout->addWidget(new QLabel(weightUnitsLabel));

	formLayout->addRow(new QLabel("Interval:"), intervalLayout);

	direction = new QComboBox();
	direction->addItem("Values increasing", QVariant(true));
	direction->addItem("Values decreasing", QVariant(false));
	direction->resize(direction->sizeHint());
	direction->setFixedWidth(direction->sizeHint().width());

	formLayout->addRow(new QLabel("Direction:"), direction);

	QLabel *label = new QLabel("Automatically fill in charge weights for all enabled series, starting from the top of the list.\n");
	label->setWordWrap(true);

	QVBoxLayout *layout = new QVBoxLayout();
	layout->addWidget(label);
	layout->addLayout(formLayout);
	layout->addWidget(direction);
	layout->addWidget(buttonBox);

	setLayout(layout);
	setFixedSize(sizeHint());
}

AutofillValues *AutofillDialog::getValues(void)
{
	AutofillValues *values = new AutofillValues();
	values->startingCharge = startingCharge->value();
	values->interval = interval->value();
	values->increasing = direction->currentData().value<bool>();

	return values;
}

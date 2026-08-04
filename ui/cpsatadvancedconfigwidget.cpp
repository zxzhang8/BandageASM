#include "cpsatadvancedconfigwidget.h"

#include <QDoubleSpinBox>
#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>
#include "infotextwidget.h"

namespace
{
QDoubleSpinBox *makeDoubleSpinBox(double minimum, double maximum, int decimals,
                                  double step, QWidget *parent)
{
    QDoubleSpinBox *box = new QDoubleSpinBox(parent);
    box->setRange(minimum, maximum);
    box->setDecimals(decimals);
    box->setSingleStep(step);
    return box;
}
}

CpSatAdvancedConfigWidget::CpSatAdvancedConfigWidget(
        QWidget *parent, const TanglePathParameters &parameters) :
    QWidget(parent),
    m_coverageDispersion(makeDoubleSpinBox(0.0, 10.0, 4, 0.05, this)),
    m_huberDelta(makeDoubleSpinBox(0.0, 1000.0, 4, 0.25, this)),
    m_fullThreadFraction(makeDoubleSpinBox(0.0, 1.0, 4, 0.05, this)),
    m_contextFraction(makeDoubleSpinBox(0.0, 1.0, 4, 0.05, this)),
    m_contextMin(new QSpinBox(this)),
    m_contextMax(new QSpinBox(this)),
    m_alignmentScoreFraction(makeDoubleSpinBox(0.0, 1.0, 4, 0.01, this)),
    m_coverageWeight(makeDoubleSpinBox(0.0, 1000.0, 4, 0.1, this)),
    m_readWeight(makeDoubleSpinBox(0.0, 1000.0, 4, 0.1, this)),
    m_randomSeed(new QSpinBox(this))
{
    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    QLabel *title = new QLabel("CP-SAT advanced configuration", this);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    title->setFont(titleFont);
    outerLayout->addWidget(title);

    QLabel *description = new QLabel(
                "These settings affect CP-SAT evidence filtering and objective scoring. "
                "Changes are applied only after you click Confirm.", this);
    description->setWordWrap(true);
    outerLayout->addWidget(description);

    QLabel *formula = new QLabel(this);
    formula->setTextFormat(Qt::RichText);
    formula->setWordWrap(true);
    formula->setTextInteractionFlags(Qt::TextSelectableByMouse);
    formula->setText(
                "<b>Objective minimized by CP-SAT</b><br>"
                "J(P) = w<sub>cov</sub> &Sigma;<sub>i</sub> l<sub>i</sub> "
                "H<sub>&delta;</sub>((c<sub>i</sub> - n<sub>i</sub>c<sub>1</sub>) / "
                "(&rho;c<sub>1</sub>)) + w<sub>read</sub> &Sigma;<sub>r</sub> q<sub>r</sub> "
                "[1 - &alpha;I<sub>full,r</sub>(P) - &beta;&Sigma;<sub>k</sub> "
                "f<sub>rk</sub>I<sub>context,rk</sub>(P)] + &epsilon;(|P|-1)<br>"
                "<small>&rho; = coverage dispersion, &delta; = coverage Huber delta, "
                "&alpha; = full-thread fraction, &beta; = context fraction, "
                "w<sub>cov</sub> = coverage weight, and w<sub>read</sub> = read evidence weight. "
                "Contexts use lengths [context min, context max]; alignments are retained at "
                "score &ge; f<sub>AS</sub> &times; best score.</small>");
    formula->setStyleSheet("QLabel { background: palette(alternate-base); border: 1px solid "
                           "palette(mid); padding: 8px; }");
    outerLayout->addWidget(formula);

    QWidget *formWidget = new QWidget(this);
    QGridLayout *form = new QGridLayout(formWidget);
    form->setColumnStretch(2, 1);
    int row = 0;
    const auto addParameter = [&](const QString &label, QWidget *field, const QString &help)
    {
        InfoTextWidget *info = new InfoTextWidget(formWidget, help);
        form->addWidget(info, row, 0, Qt::AlignVCenter);
        form->addWidget(new QLabel(label, formWidget), row, 1);
        form->addWidget(field, row, 2);
        ++row;
    };
    addParameter("Coverage dispersion (ρ):", m_coverageDispersion,
                 "Sets &rho;, the expected relative coverage dispersion. The coverage residual "
                 "for node i is divided by &rho; times the estimated single-copy coverage. "
                 "Larger values tolerate more coverage variation.");
    addParameter("Coverage Huber delta (δ):", m_huberDelta,
                 "Sets the transition point &delta; of the Huber loss H<sub>&delta;</sub> used "
                 "for coverage residuals. Residuals below this point are quadratic; larger "
                 "residuals are penalized linearly.");
    addParameter("Full-thread fraction (α):", m_fullThreadFraction,
                 "Sets &alpha;, the reward assigned when the candidate path contains a retained "
                 "read's complete graph thread (in either orientation).");
    addParameter("Context fraction (β):", m_contextFraction,
                 "Sets &beta;, the total reward available from matching shorter read contexts. "
                 "Each context receives a normalized fraction of this reward.");
    m_contextMin->setRange(0, 1000000);
    m_contextMax->setRange(0, 1000000);
    addParameter("Minimum context nodes:", m_contextMin,
                 "Shortest contiguous read subpath used as context evidence. A value of 2 means "
                 "single-node observations are ignored.");
    addParameter("Maximum context nodes:", m_contextMax,
                 "Longest contiguous read subpath used as context evidence. If a read thread is "
                 "shorter, its full available length is used.");
    addParameter("Alignment-score fraction (f<sub>AS</sub>):", m_alignmentScoreFraction,
                 "After keeping alignments with the best mapping quality, retain alternatives "
                 "whose alignment score is at least f<sub>AS</sub> times the best score for that read.");
    addParameter("Coverage weight (w<sub>cov</sub>):", m_coverageWeight,
                 "Multiplier w<sub>cov</sub> for the coverage-fit term in the objective. Increase "
                 "it to favor paths whose node copy counts better match coverage.");
    addParameter("Read evidence weight (w<sub>read</sub>):", m_readWeight,
                 "Multiplier w<sub>read</sub> for full-thread and context evidence. Increase it "
                 "to make read support more influential relative to coverage fit.");
    m_randomSeed->setRange(0, 2147483647);
    addParameter("Random seed:", m_randomSeed,
                 "Seed passed to the CP-SAT solver. It does not appear in the objective formula, "
                 "but controls deterministic solver search choices when alternatives exist.");
    form->setRowStretch(row, 1);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(formWidget);
    outerLayout->addWidget(scrollArea, 1);

    QHBoxLayout *buttons = new QHBoxLayout();
    QPushButton *resetButton = new QPushButton("Restore defaults", this);
    QPushButton *cancelButton = new QPushButton("Cancel", this);
    QPushButton *confirmButton = new QPushButton("Confirm", this);
    confirmButton->setDefault(true);
    buttons->addWidget(resetButton);
    buttons->addStretch();
    buttons->addWidget(cancelButton);
    buttons->addWidget(confirmButton);
    outerLayout->addLayout(buttons);

    setValues(parameters);
    connect(resetButton, SIGNAL(clicked()), this, SLOT(resetDefaults()));
    connect(cancelButton, SIGNAL(clicked()), this, SIGNAL(closeRequested()));
    connect(confirmButton, SIGNAL(clicked()), this, SLOT(applyAndClose()));
}

void CpSatAdvancedConfigWidget::setValues(const TanglePathParameters &parameters)
{
    m_coverageDispersion->setValue(parameters.coverageDispersion);
    m_huberDelta->setValue(parameters.cpHuberDelta);
    m_fullThreadFraction->setValue(parameters.fullThreadFraction);
    m_contextFraction->setValue(parameters.contextFraction);
    m_contextMin->setValue(parameters.contextMin);
    m_contextMax->setValue(parameters.contextMax);
    m_alignmentScoreFraction->setValue(parameters.asFraction);
    m_coverageWeight->setValue(parameters.coverageWeight);
    m_readWeight->setValue(parameters.readWeight);
    m_randomSeed->setValue(parameters.randomSeed);
}

TanglePathParameters CpSatAdvancedConfigWidget::values() const
{
    TanglePathParameters parameters;
    parameters.coverageDispersion = m_coverageDispersion->value();
    parameters.cpHuberDelta = m_huberDelta->value();
    parameters.fullThreadFraction = m_fullThreadFraction->value();
    parameters.contextFraction = m_contextFraction->value();
    parameters.contextMin = m_contextMin->value();
    parameters.contextMax = m_contextMax->value();
    parameters.asFraction = m_alignmentScoreFraction->value();
    parameters.coverageWeight = m_coverageWeight->value();
    parameters.readWeight = m_readWeight->value();
    parameters.randomSeed = m_randomSeed->value();
    return parameters;
}

void CpSatAdvancedConfigWidget::applyAndClose()
{
    if (m_contextMin->value() > m_contextMax->value())
        m_contextMax->setValue(m_contextMin->value());
    emit accepted(values());
    emit closeRequested();
}

void CpSatAdvancedConfigWidget::resetDefaults()
{
    setValues(TanglePathParameters());
}

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
    m_tauMin(makeDoubleSpinBox(0.0001, 1.0, 4, 0.05, this)),
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
    QLabel *title = new QLabel("RCAP advanced configuration", this);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    title->setFont(titleFont);
    outerLayout->addWidget(title);

    QLabel *description = new QLabel(
                "These settings affect RCAP evidence filtering and objective scoring. "
                "Changes are applied only after you click Confirm.", this);
    description->setWordWrap(true);
    outerLayout->addWidget(description);

    QLabel *formula = new QLabel(this);
    formula->setTextFormat(Qt::RichText);
    formula->setWordWrap(true);
    formula->setTextInteractionFlags(Qt::TextSelectableByMouse);
    formula->setText(
                "<b>Objective minimized by RCAP using CP-SAT</b><br>"
                "J(P) = w'<sub>cov</sub> &Sigma;<sub>i</sub> l<sub>i</sub> "
                "H<sub>&delta;</sub>((c<sub>i</sub> - n<sub>i</sub>c<sub>1</sub>) / "
                "(&rho;c<sub>1</sub>)) + w'<sub>read</sub> &Sigma;<sub>r</sub> q<sub>r</sub> "
                "[1 - &alpha;I<sub>full,r</sub>(P) - &beta;&Sigma;<sub>k</sub> "
                "f<sub>rk</sub>I<sub>context,rk</sub>(P)] + &epsilon;(|P|-1)<br>"
                "<small>q&#772; = mean<sub>r</sub>(q<sub>r</sub>), "
                "f<sub>read</sub> = clamp(w<sub>read</sub> / (w<sub>cov</sub> + "
                "w<sub>read</sub>) + 0.375(q&#772; - 0.5), 0.1, 0.9), "
                "w'<sub>read</sub> = (w<sub>cov</sub> + w<sub>read</sub>)f<sub>read</sub>, "
                "w'<sub>cov</sub> = w<sub>cov</sub> + w<sub>read</sub> - w'<sub>read</sub>.</small><br>"
                "<small>c<sub>1</sub> = single-copy coverage, "
                "&rho; = coverage dispersion, &delta; = coverage Huber delta, "
                "&alpha; = full-thread fraction, &beta; = context fraction, "
                "The configured base weights w<sub>cov</sub> and w<sub>read</sub> are adjusted "
                "to effective weights w'<sub>cov</sub> and w'<sub>read</sub> from the mean retained-read "
                "confidence while preserving their total. "
                "Contexts use lengths [context min, context max]; evidence is weighted by "
                "AS, identity, and mapping ambiguity; matching rewards are capped by expected "
                "pattern counts.</small>");
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
    addParameter("RCAP minimum coverage ratio (τ):", m_tauMin,
                 "Sets the minimum expected fraction of single-copy coverage used to derive "
                 "RCAP node visit bounds. This parameter is specific to RCAP and does "
                 "not change Beam search.");
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
    addParameter("Base coverage weight (w<sub>cov</sub>):", m_coverageWeight,
                 "Base multiplier for coverage fit. RCAP dynamically adjusts it against the read "
                 "weight using mean retained-read confidence while preserving their total.");
    addParameter("Base read evidence weight (w<sub>read</sub>):", m_readWeight,
                 "Base multiplier for full-thread and context evidence. RCAP increases its effective "
                 "share for higher-quality retained reads and decreases it for lower-quality reads.");
    m_randomSeed->setRange(0, 2147483647);
    addParameter("Random seed:", m_randomSeed,
                 "Seed passed to RCAP's CP-SAT solver. It does not appear in the objective formula, "
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
    m_tauMin->setValue(parameters.cpTauMin);
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
    parameters.cpTauMin = m_tauMin->value();
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

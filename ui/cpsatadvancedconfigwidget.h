#ifndef CPSATADVANCEDCONFIGWIDGET_H
#define CPSATADVANCEDCONFIGWIDGET_H

#include <QWidget>
#include "../program/tanglepathsearch.h"

class QDoubleSpinBox;
class QSpinBox;

class CpSatAdvancedConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CpSatAdvancedConfigWidget(QWidget *parent,
                                       const TanglePathParameters &parameters);

signals:
    void accepted(const TanglePathParameters &parameters);
    void closeRequested();

private slots:
    void applyAndClose();
    void resetDefaults();

private:
    void setValues(const TanglePathParameters &parameters);
    TanglePathParameters values() const;

    QDoubleSpinBox *m_coverageDispersion;
    QDoubleSpinBox *m_huberDelta;
    QDoubleSpinBox *m_tauMin;
    QDoubleSpinBox *m_fullThreadFraction;
    QDoubleSpinBox *m_contextFraction;
    QSpinBox *m_contextMin;
    QSpinBox *m_contextMax;
    QDoubleSpinBox *m_alignmentScoreFraction;
    QDoubleSpinBox *m_coverageWeight;
    QDoubleSpinBox *m_readWeight;
    QSpinBox *m_randomSeed;
};

#endif // CPSATADVANCEDCONFIGWIDGET_H

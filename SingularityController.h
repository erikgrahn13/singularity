#pragma once

#include "IParameterBackend.h"

#include "AudioDataExchange.h"

#include <QMetaObject>
#include <QPointer>
#include <QQuickView>
#include <QHash>
#include <QUrl>

class SingularityController;

class QmlParameter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)

public:
    QmlParameter(int id, SingularityController& controller, QObject* parent = nullptr);

    double value() const;
    void setValue(double value);
    void notifyChanged();

signals:
    void valueChanged();

private:
    int id_;
    SingularityController& controller_;
};



class SingularityController : public QObject
{
    Q_OBJECT
public:
    SingularityController(IParameterBackend& parameterBackend);
    ~SingularityController();

    void attachToView(QQuickView& view, const QUrl& source = {});
    void detachView();

    Q_INVOKABLE QObject* get(int id) const;

private:
    friend class QmlParameter;

    double getParameterValue(int id) const;
    void setParameterValue(int id, double value);
    QPointer<QQuickView> view_;
    QMetaObject::Connection statusConnection_;
    IParameterBackend& parameterBackend_;
    QHash<int, QmlParameter*> qmlParameters_;
};

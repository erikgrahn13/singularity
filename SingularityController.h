#pragma once

#include "IParameterBackend.h"

#include "AudioDataExchange.h"

#include <functional>
#include <string>
#include <string_view>

#include <QFileSystemWatcher>
#include <QMetaObject>
#include <QPointer>
#include <QQuickView>
#include <QString>
#include <QTimer>
#include <QHash>

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
    // Q_PROPERTY(int id READ id CONSTANT)
    // Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
public:
    using LogCallback = std::function<void(const std::string&)>;

    // SingularityController(IParameterProvider& parameterProvider, std::span<const Parameter> definitions);
    SingularityController(IParameterBackend& parameterBackend);
    ~SingularityController();

    void setLogger(LogCallback callback);
    void attachToView(QQuickView& view);
    void detachView();

    // Q_INVOKABLE QObject* parameter(int id) const;
    Q_INVOKABLE QObject* get(int id) const;
    // Q_INVOKABLE void setParameter(int id, double value);
    // double value();
    // void setValue(double value);


// signals:
    // void parameterChanged(int id, double value);
    // void valueChanged(double value);

private:
    friend class QmlParameter;

    double getParameterValue(int id) const;
    void setParameterValue(int id, double value);
    QPointer<QQuickView> view_;
    QFileSystemWatcher qmlWatcher_;
    QTimer reloadTimer_;
    QString qmlFile_;
    QMetaObject::Connection statusConnection_;
    LogCallback logger_;
    // IParameterProvider& parameterProvider_;
    IParameterBackend& parameterBackend_;
    QHash<int, QmlParameter*> qmlParameters_;
};

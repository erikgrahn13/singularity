#pragma once

#include "IParameterBackend.h"

#include "AudioDataExchange.h"

#include <QHash>
#include <QMetaObject>
#include <QPointer>
#include <QQuickView>
#include <QString>
#include <QUrl>
#include <QVariant>

#include <functional>
#include <string_view>

class QmlParameter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)

public:
    explicit QmlParameter(
        int id,
        IParameterBackend& backend,
        QObject* parent = nullptr);

    double value() const;
    void setValue(double value);
    void notifyChanged();

signals:
    void valueChanged();

private:
    int id_;
    IParameterBackend& backend_;
};

class QmlParameterCollection : public QObject
{
    Q_OBJECT

public:
    explicit QmlParameterCollection(
        IParameterBackend& backend,
        QObject* parent = nullptr);

    Q_INVOKABLE QmlParameter* get(int id) const;

private:
    QHash<int, QmlParameter*> parameters_;
};

class SingularityController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(
        QmlParameterCollection* parameters
        READ parameters
        CONSTANT)

public:
    using ActionCallback =
        std::function<void(std::string_view, std::string_view)>;

    SingularityController(
        IParameterBackend& parameterBackend,
        ActionCallback actionCallback);
    ~SingularityController();

    void attachToView(QQuickView& view, const QUrl& source = {});
    void detachView();

    QmlParameterCollection* parameters();

    Q_INVOKABLE void sendAction(QString name);
    Q_INVOKABLE void sendAction(QString name, QVariant value);

private:
    QPointer<QQuickView> view_;
    QMetaObject::Connection statusConnection_;
    ActionCallback actionCallback_;
    QmlParameterCollection parameterCollection_;
};

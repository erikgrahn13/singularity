#pragma once

#include "SingularityPlugin.h"

#include <QHash>
#include <QList>
#include <QMetaObject>
#include <QPointer>
#include <QQuickView>
#include <QString>
#include <QUrl>
#include <QVariant>

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

class QmlParameterCollection;

class QmlParameter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)

public:
    explicit QmlParameter(unsigned int id, QmlParameterCollection& collection, QObject* parent = nullptr);

    double value() const;
    void setValue(double value);
    void notifyChanged();

signals:
    void valueChanged();

private:
    int id_;
    QmlParameterCollection& collection_;
};

class QmlParameterCollection : public QObject
{
    Q_OBJECT

public:
    using Getter = std::function<double(unsigned int)>;
    using Setter = std::function<void(unsigned int, double)>;

    explicit QmlParameterCollection(Getter getter, Setter setter, QObject* parent = nullptr);

    Q_INVOKABLE QmlParameter* get(int id);

    double value(unsigned int id) const
    {
        return getter_(id);
    }

    void setValue(unsigned int id, double value)
    {
        setter_(id, value);
    }

    void notifyChanged(unsigned int id)
    {
        if(auto* parameter = parameters_.value(id))
            parameter->notifyChanged();
    }

private:
    Getter getter_;
    Setter setter_;
    QHash<unsigned int, QmlParameter*> parameters_;
};

class SingularityController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(
        QmlParameterCollection* parameters
        READ parameters
        CONSTANT)
    Q_PROPERTY(
        QVariantMap data
        READ data
        NOTIFY dataChanged)

public:
    using ActionCallback = std::function<ActionResult(std::string_view, const ActionValue& value)>;
    using ParameterGetter = std::function<double(unsigned int)>;
    using ParameterSetter = std::function<void(unsigned int, double)>;

    SingularityController(
        ParameterGetter getParameter,
        ParameterSetter setParameter,
        ActionCallback actionCallback);
    ~SingularityController();

    void attachToView(QQuickView& view, const QUrl& source = {});
    void detachView();

    QmlParameterCollection* parameters();
    QVariantMap data() const;
    void setData(std::string_view name, const ActionValue& value);

    Q_INVOKABLE void sendAction(QString name);
    Q_INVOKABLE void sendAction(QString name, QVariant value);

signals:
    void dataChanged();

private:
    QPointer<QQuickView> view_;
    QMetaObject::Connection statusConnection_;
    ActionCallback actionCallback_;
    ActionCallback resultCallback_;
    QmlParameterCollection parameterCollection_;
    QVariantMap data_;
};

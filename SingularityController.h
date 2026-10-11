#pragma once

#include "UiDataExchange.h"
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

class QmlUiDataStreamCollection;

class QmlUiDataStream : public QObject
{
    Q_OBJECT
    Q_PROPERTY(quint32 revision READ revision NOTIFY dataChanged)
    Q_PROPERTY(double sampleRate READ sampleRate NOTIFY dataChanged)
    Q_PROPERTY(quint32 rows READ rows NOTIFY dataChanged)
    Q_PROPERTY(quint32 columns READ columns NOTIFY dataChanged)
    Q_PROPERTY(QList<float> values READ values NOTIFY dataChanged)

public:
    explicit QmlUiDataStream(
        quint32 streamId,
        QmlUiDataStreamCollection& collection,
        QObject* parent = nullptr);

    quint32 revision() const;
    double sampleRate() const;
    quint32 rows() const;
    quint32 columns() const;
    QList<float> values() const;

    Q_INVOKABLE bool update();

signals:
    void dataChanged();

private:
    friend class QmlUiDataStreamCollection;
    void apply(const Singularity::UiDataExchange::DataBlock& block);

    quint32 streamId_;
    QmlUiDataStreamCollection& collection_;
    quint32 revision_ = 0;
    quint32 observedRevision_ = 0;
    double sampleRate_ = 0.0;
    quint32 rows_ = 0;
    quint32 columns_ = 0;
    QList<float> values_;
};

class QmlUiDataStreamCollection : public QObject
{
    Q_OBJECT

public:
    explicit QmlUiDataStreamCollection(
        Singularity::UiDataExchange::DataQueue* queue,
        QObject* parent = nullptr);

    Q_INVOKABLE QmlUiDataStream* get(int streamId);
    bool update();

private:
    Singularity::UiDataExchange::DataQueue* queue_;
    QHash<quint32, QmlUiDataStream*> streams_;
};

class SingularityController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(
        QmlParameterCollection* parameters
        READ parameters
        CONSTANT)
    Q_PROPERTY(
        QmlUiDataStreamCollection* streams
        READ streams
        CONSTANT)
    Q_PROPERTY(
        QVariantMap data
        READ data
        NOTIFY dataChanged)

public:
    // using ActionResult = std::optional<std::pair<std::string, std::string>>;
    // using ActionValue = std::variant<std::monostate, bool, std::int64_t, double, std::string>;
    using ActionCallback = std::function<ActionResult(std::string_view, const ActionValue& value)>;
    using ParameterGetter = std::function<double(unsigned int)>;
    using ParameterSetter = std::function<void(unsigned int, double)>;

    SingularityController(
        ParameterGetter getParameter,
        ParameterSetter setParameter,
        ActionCallback actionCallback,
        Singularity::UiDataExchange::DataQueue* uiDataQueue = nullptr);
    ~SingularityController();

    void attachToView(QQuickView& view, const QUrl& source = {});
    void detachView();

    QmlParameterCollection* parameters();
    QmlUiDataStreamCollection* streams();
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
    QmlUiDataStreamCollection streamCollection_;
    QVariantMap data_;
};

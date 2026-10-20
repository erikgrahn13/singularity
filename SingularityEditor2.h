#pragma once

#include "SingularityPlugin.h"

#include <QQuickView>
#include <QObject>
#include <unordered_map>

class QmlParameter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged);
public:
    using Getter = std::function<double(unsigned int)>;
    using Setter = std::function<void(unsigned int, double)>;

    explicit QmlParameter(unsigned int id, Getter getter, Setter setter, QObject* parent = nullptr);
    double value() const;
    void setValue(double value);

    signals:
    void valueChanged();
private:
    int id_;
    Getter getter_;
    Setter setter_;
};

class SingularityEditor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap data READ data NOTIFY dataChanged)

    public:
    explicit SingularityEditor(const char* platform = nullptr, std::string hotReloadArgument = {});
    ~SingularityEditor() override;

    QVariantMap data() const;
    void setData(std::string_view name, const ActionValue& value);

    virtual double getParameter(unsigned int ) = 0;
    virtual void setParameter(unsigned int id, double value) = 0;

    Q_INVOKABLE QmlParameter* parameter(int id);
    Q_INVOKABLE void sendAction(QString name, QVariant value);

    signals:
    void dataChanged();

    protected:
    virtual void requestResize(int width, int height) = 0;
    virtual void dispatchAction(std::string_view name, const ActionValue& value) = 0;
    void initializeView();
    void showWindow();
    QQuickView& getView() { return *view_; }

    private:
    static QGuiApplication  application;
    std::unique_ptr<QQuickView> view_;
    std::unordered_map<int, QmlParameter*> parameters_;
    QVariantMap data_;
};
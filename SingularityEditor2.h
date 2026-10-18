#pragma once

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
    public:
    explicit SingularityEditor(const char* platform = nullptr, std::string hotReloadArgument = {});
    ~SingularityEditor() override;

    virtual double getParameter(unsigned int ) = 0;
    virtual void setParameter(unsigned int id, double value) = 0;

    Q_INVOKABLE QmlParameter* parameter(int id);

    protected:
    void initializeView();
    void showWindow();
    QQuickView& getView() { return *view_; }

    private:
    static QGuiApplication  application;
    std::unique_ptr<QQuickView> view_;
    std::unordered_map<int, QmlParameter*> parameters_;
};
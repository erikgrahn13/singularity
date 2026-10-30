#pragma once

#include "SingularityPlugin.h"
#include "utilities/SingularityCommunication.h"

#include <QQuickView>
#include <QObject>
#include <optional>
#include <unordered_map>

class SingularityEditor;

class QmlParameter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged);
    Q_PROPERTY(double minValue READ minValue CONSTANT);
    Q_PROPERTY(double maxValue READ maxValue CONSTANT);

public:
    using Getter = std::function<double(unsigned int)>;
    using Setter = std::function<void(unsigned int, double)>;

    explicit QmlParameter(unsigned int id, double minValue, double maxValue, SingularityEditor& editor, QObject* parent = nullptr);
    double value() const;
    double minValue() const;
    double maxValue() const;
    void setValue(double value);
    unsigned int id() const { return id_; }

    signals:
    void valueChanged();
private:
    int id_;
    double minValue_;
    double maxValue_;
    SingularityEditor& editor_;
};

class SingularityEditor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap parameterIds READ parameterIds CONSTANT)
    Q_PROPERTY(QVariantMap data READ data NOTIFY dataChanged)
    Q_PROPERTY(QVariantList audioSamples READ audioSamples NOTIFY audioChanged)


    public:
    explicit SingularityEditor(const char* platform = nullptr, std::string hotReloadArgument = {});
    ~SingularityEditor() override;

    QVariantMap parameterIds() const;

    QVariantMap data() const;
    void setData(std::string_view name, const ActionValue& value);
    void notifyParameterChanged(unsigned int id);
    std::optional<unsigned int> hitTestHelper(int xPos, int yPos);

    QVariantList audioSamples() const;

    virtual double getParameter(unsigned int ) = 0;
    virtual void setParameter(unsigned int id, double value) = 0;
    virtual bool popUiAudioBlock(UiAudioBlock& block) = 0;

    Q_INVOKABLE QmlParameter* parameter(int id);
    Q_INVOKABLE void sendAction(QString name, QVariant value);

    signals:
    void dataChanged();
    void audioChanged();

    protected:
    virtual void requestResize(int width, int height) = 0;
    virtual void dispatchAction(std::string_view name, const ActionValue& value) = 0;
    void updateAudio();
    void initializeView();
    void showWindow();
    QQuickView& getView() { return *view_; }

    private:
    static QGuiApplication  application;
    std::unique_ptr<QQuickView> view_;
    std::unordered_map<int, QmlParameter*> parameters_;
    QVariantMap data_;
    QVariantList audioSamples_;
};

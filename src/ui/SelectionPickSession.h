#pragma once

#include <QObject>
#include <QPointer>

#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS_Shape.hxx>

#include <map>
#include <vector>

class OCCView;

// Owns the complete viewer-picking lifecycle for a tool window.
class SelectionPickSession final : public QObject
{
    Q_OBJECT

public:
    struct Options
    {
        std::vector<TopAbs_ShapeEnum> acceptedTypes;
        bool clearExistingSelection{true};
        bool exclusiveFilters{true};
    };

    explicit SelectionPickSession(QObject* parent = nullptr);
    ~SelectionPickSession() override;

    bool start(const Options& options);
    void stop();
    bool isActive() const;
    OCCView* view() const;

signals:
    void shapePicked(const TopoDS_Shape& shape);
    void activeChanged(bool active);

private:
    void restoreViewState();

    QPointer<OCCView> m_view;
    int m_savedMouseMode{0};
    std::map<TopAbs_ShapeEnum, bool> m_savedFilters;
    bool m_active{false};
};

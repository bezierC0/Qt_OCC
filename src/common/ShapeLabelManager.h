#pragma once

#include <TopoDS_Shape.hxx>
#include <TDF_Label.hxx>
#include <unordered_map>
#include <mutex>
#include <TopoDS.hxx>
#include <functional>

struct ShapeHash {
    std::size_t operator()(const TopoDS_Shape& s) const {
        return std::hash<void*>{}(s.TShape().get());
    }
};

struct ShapeEqual {
    bool operator()(const TopoDS_Shape& s1, const TopoDS_Shape& s2) const {
        return s1.IsSame(s2);
    }
};

class ShapeLabelManager
{
public:
    static ShapeLabelManager& GetInstance();

    void Register(const TopoDS_Shape& shape, const TDF_Label& label);
    TDF_Label GetLabel(const TopoDS_Shape& shape) const;
    void Clear();

private:
    ShapeLabelManager() = default;
    ~ShapeLabelManager() = default;
    ShapeLabelManager(const ShapeLabelManager&) = delete;
    ShapeLabelManager& operator=(const ShapeLabelManager&) = delete;

    std::unordered_map<TopoDS_Shape, TDF_Label, ShapeHash, ShapeEqual> m_map;
    mutable std::mutex m_mutex;
};

#include "ShapeLabelManager.h"

ShapeLabelManager& ShapeLabelManager::GetInstance()
{
    static ShapeLabelManager instance;
    return instance;
}

void ShapeLabelManager::Register(const TopoDS_Shape& shape, const TDF_Label& label)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!shape.IsNull() && !label.IsNull()) {
        m_map[shape] = label;
    }
}

TDF_Label ShapeLabelManager::GetLabel(const TopoDS_Shape& shape) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_map.find(shape);
    if (it != m_map.end()) {
        return it->second;
    }
    return TDF_Label();
}

void ShapeLabelManager::Clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_map.clear();
}

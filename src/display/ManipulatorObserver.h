#pragma once
class gp_Trsf;
class ManipulatorObserver
{
public:
    virtual ~ManipulatorObserver() = default;
    virtual void onManipulatorChange(const gp_Trsf& trsf) = 0;
};

#pragma once

#include "SingularityEditor2.h"

class SingularityAPPEditor : public SingularityEditor
{
public:
    double getParameter(unsigned int id) override;
    void setParameter(unsigned int id, double value) override;
    void requestResize(int width, int height) override;
};
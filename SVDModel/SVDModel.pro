TEMPLATE = subdirs

SUBDIRS += \
    SVDCore \
    Predictor \
    SVDUI

SVDUI.depends = SVDCore Predictor

RESOURCES += \
    SVDUI/res/iland.qrc

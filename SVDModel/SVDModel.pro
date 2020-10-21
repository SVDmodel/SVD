TEMPLATE = subdirs

SUBDIRS += \
    SVDCore \
    Predictor \
    SVDUI \
    SVDc

SVDUI.depends = SVDCore Predictor
SVDc.depends = SVDCore Predictor

RESOURCES += \
    SVDUI/res/resource.qrc

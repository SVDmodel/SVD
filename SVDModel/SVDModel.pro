TEMPLATE = subdirs

SUBDIRS += \
    SVDCore \
    Predictor \
    SVDUI

SVDUI.depends = SVDCore Predictor

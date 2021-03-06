# -*- coding: utf-8 -*-

"""
***************************************************************************
    pksvm.py
    ---------------------
    Date                 : April 2016
    Copyright            : (C) 2016 by Pieter Kempeneers
    Email                : kempenep at gmail dot com
***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************
"""

__author__ = 'Pieter Kempeneers'
__date__ = 'April 2016'
__copyright__ = '(C) 2016, Pieter Kempeneers'
# This will get replaced with a git SHA1 when you do a git archive
__revision__ = '$Format:%H$'

import os
from pktoolsUtils import pktoolsUtils
from pktoolsAlgorithm import pktoolsAlgorithm
from processing.core.parameters import ParameterMultipleInput
from processing.core.parameters import ParameterRaster
from processing.core.outputs import OutputRaster
from processing.core.parameters import ParameterSelection
from processing.core.parameters import ParameterFile
from processing.core.parameters import ParameterNumber
from processing.core.parameters import ParameterString
from processing.core.parameters import ParameterBoolean
from processing.core.parameters import ParameterExtent

class pksvm(pktoolsAlgorithm):

    INPUT = "INPUT"
    TRAINING = "TRAINING"
    ITERATE = "ITERATE"
    LABEL = "LABEL"
#    CV = "CV"
    GAMMA = "GAMMA"
    COST = "COST"
    OUTPUT = "OUTPUT"
    MASK = "MASK"
    MSKNODATA = "MSKNODATA"
#    NODATA = "NODATA"

#    SVM_TYPE_OPTIONS = ["C_SVC", "nu_SVC,one_class", "epsilon_SVR", "nu_SVR"]
#    KERNEL_TYPE_OPTIONS = ["linear", "polynomial", "radial", "sigmoid"]
    EXTRA = 'EXTRA'

    def cliName(self):
        return "pksvm"

    def defineCharacteristics(self):
        self.name = "Support vector machine"
        self.group = "[pktools] supervised classification"
        self.addParameter(ParameterRaster(self.INPUT, 'Input layer raster data set',ParameterRaster))
        self.addParameter(ParameterFile(self.TRAINING, "Training vector file", False, optional=False))
        self.addParameter(ParameterString(self.LAYERS, "Layer name(s) in sample (leave empty to select all",'', optional=True))
        self.addParameter(ParameterBoolean(self.ITERATE, "Iterate over all layers",True))
        self.addParameter(ParameterString(self.LABEL, "Attribute name for class label in training vector file","label"))
        self.addParameter(ParameterNumber(self.GAMMA, "Gamma in kernel function",0,100,1.0))
        self.addParameter(ParameterNumber(self.COST, "The parameter C of C_SVC",0,100000,1000.0))
        self.addParameter(ParameterFile(self.MASK, "Mask vector/raster dataset used for classification","None",optional=True))
        self.addParameter(ParameterString(self.MSKNODATA, "Mask value(s) not to consider for classification (in case of raster mask, e.g., 0;255)","0"))
        self.addOutput(OutputRaster(self.OUTPUT, "Output raster data set"))
        self.addParameter(ParameterString(self.EXTRA,
                          'Additional parameters', '-of GTiff', optional=True))

#        self.addParameter(ParameterSelection(self.KERNEL_TYPE,"Type of kernel function (linear,polynomial,radial,sigmoid)",self.KERNEL_TYPE_OPTIONS, 2))
#        self.addParameter(ParameterSelection(self.SVM_TYPE,"Type of SVM (C_SVC, nu_SVC,one_class, epsilon_SVR, nu_SVR)",self.SVM_TYPE_OPTIONS, 0))

    def processAlgorithm(self, progress):
        cliPath = '"' + os.path.join(pktoolsUtils.pktoolsPath(), self.cliName()) + '"'
        commands = [cliPath]

        input=self.getParameterValue(self.INPUT)
        if input != "":
            commands.append('-i')
            commands.append('"' + input + '"')

        commands.append('-t')
        training=self.getParameterValue(self.TRAINING)
        commands.append(training)

        layer=self.getParameterValue(self.LAYERS)
        if layer != '':
            layerValues = layer.split(';')
            for layerValue in layerValues:
                commands.append('-ln')
                commands.append(layerValue)

        commands.append('-label')
        commands.append(str(self.getParameterValue(self.LABEL)))
        # if self.getParameterValue(self.CV):
        #     commands.append("-cv 2")
        commands.append('-g')
        commands.append(str(self.getParameterValue(self.GAMMA)))
        commands.append('-cc')
        commands.append(str(self.getParameterValue(self.COST)))

        mask = str(self.getParameterValue(self.MASK))
        if mask != "":
            commands.append('-m')
            commands.append(mask)
            msknodata=str(self.getParameterValue(self.MSKNODATA))
            msknodataValues = msknodata.split(';')
            for msknodataValue in msknodataValues:
                commands.append('-msknodata')
                commands.append(msknodataValue)

        extra = str(self.getParameterValue(self.EXTRA))
        if len(extra) > 0:
            commands.append(extra)

        output=self.getOutputValue(self.OUTPUT)
        if output != "":
            commands.append('-o')
            commands.append('"' + output + '"')

        pktoolsUtils.runpktools(commands, progress)

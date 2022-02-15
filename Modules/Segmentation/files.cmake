set(CPP_FILES
  Algorithms/mitkCalculateSegmentationVolume.cpp
  Algorithms/mitkContourModelSetToImageFilter.cpp
  Algorithms/mitkContourSetToPointSetFilter.cpp
  Algorithms/mitkContourUtils.cpp
  Algorithms/mitkCorrectorAlgorithm.cpp
  Algorithms/mitkDiffImageApplier.cpp
  Algorithms/mitkDiffSliceOperation.cpp
  Algorithms/mitkDiffSliceOperationApplier.cpp
  Algorithms/mitkFeatureBasedEdgeDetectionFilter.cpp
  Algorithms/mitkImageLiveWireContourModelFilter.cpp
  Algorithms/mitkImageToContourFilter.cpp
  #Algorithms/mitkImageToContourModelFilter.cpp
  Algorithms/mitkImageToLiveWireContourFilter.cpp
  Algorithms/mitkManualSegmentationToSurfaceFilter.cpp
  Algorithms/mitkOtsuSegmentationFilter.cpp
  Algorithms/mitkSegmentationObjectFactory.cpp
  Algorithms/mitkShapeBasedInterpolationAlgorithm.cpp
  Algorithms/mitkShowSegmentationAsSmoothedSurface.cpp
  Algorithms/mitkShowSegmentationAsSurface.cpp
  Algorithms/mitkVtkImageOverwrite.cpp
  Controllers/mitkSegmentationInterpolationController.cpp
  Controllers/mitkToolManager.cpp
  Controllers/mitkSegmentationModuleActivator.cpp
  Controllers/mitkToolManagerProvider.cpp
  DataManagement/mitkContour.cpp
  DataManagement/mitkContourSet.cpp
  DataManagement/mitkExtrudedContour.cpp
  Interactions/mitkAdaptiveRegionGrowingTool.cpp
  Interactions/mitkAddContourTool.cpp
  Interactions/mitkAutoCropTool.cpp
  Interactions/mitkAutoSegmentationTool.cpp
  Interactions/mitkAutoSegmentationWithPreviewTool.cpp
  Interactions/mitkAutoMLSegmentationWithPreviewTool.cpp
  Interactions/mitkBinaryThresholdBaseTool.cpp
  Interactions/mitkBinaryThresholdTool.cpp
  Interactions/mitkBinaryThresholdULTool.cpp
  Interactions/mitkCalculateGrayValueStatisticsTool.cpp
  Interactions/mitkCalculateVolumetryTool.cpp
  Interactions/mitkContourModelInteractor.cpp
  Interactions/mitkContourModelLiveWireInteractor.cpp
  Interactions/mitkLiveWireTool2D.cpp
  Interactions/mitkContourTool.cpp
  Interactions/mitkCreateSurfaceTool.cpp
  Interactions/mitkDrawPaintbrushTool.cpp
  Interactions/mitkErasePaintbrushTool.cpp
  Interactions/mitkEraseRegionTool.cpp
  Interactions/mitkFastMarchingBaseTool.cpp
  Interactions/mitkFastMarchingTool.cpp
  Interactions/mitkFastMarchingTool3D.cpp
  Interactions/mitkFeedbackContourTool.cpp
  Interactions/mitkFillRegionTool.cpp
  Interactions/mitkOtsuTool3D.cpp
  Interactions/mitkPaintbrushTool.cpp
  Interactions/mitkPixelManipulationTool.cpp
  Interactions/mitkRegionGrowingTool.cpp
  Interactions/mitkSegmentationsProcessingTool.cpp
  Interactions/mitkSetRegionTool.cpp
  Interactions/mitkSegTool2D.cpp
  Interactions/mitkSubtractContourTool.cpp
  Interactions/mitkTool.cpp
  Interactions/mitkToolCommand.cpp
  Interactions/mitkWatershedTool.cpp
  Interactions/mitkPickingTool.cpp
  Interactions/mitknnUnetTool.cpp
  Interactions/mitkSegmentationInteractor.cpp #SO
  Interactions/mitkProcessExecutor.cpp
  Rendering/mitkContourMapper2D.cpp
  Rendering/mitkContourSetMapper2D.cpp
  Rendering/mitkContourSetVtkMapper3D.cpp
  Rendering/mitkContourVtkMapper3D.cpp
  SegmentationUtilities/BooleanOperations/mitkBooleanOperation.cpp
  SegmentationUtilities/MorphologicalOperations/mitkMorphologicalOperations.cpp
#Added from ML
  Controllers/mitkSliceBasedInterpolationController.cpp
  Algorithms/mitkSurfaceStampImageFilter.cpp
)

set(RESOURCE_FILES
  Add_48x48.png
  Add_Cursor_32x32.png
  Erase_48x48.png
  Erase_Cursor_32x32.png
  FastMarching_48x48.png
  FastMarching_Cursor_32x32.png
  Fill_48x48.png
  Fill_Cursor_32x32.png
  LiveWire_48x48.png
  LiveWire_Cursor_32x32.png
  Otsu_48x48.png
  Paint_48x48.png
  Paint_Cursor_32x32.png
  Pick_48x48.png
  RegionGrowing_48x48.png
  RegionGrowing_Cursor_32x32.png
  Subtract_48x48.png
  Subtract_Cursor_32x32.png
  Threshold_48x48.png
  TwoThresholds_48x48.png
  Watershed_48x48.png
  Watershed_Cursor_32x32.png
  Wipe_48x48.png
  Wipe_Cursor_32x32.png

  Interactions/dummy.xml
  Interactions/LiveWireTool.xml
  Interactions/FastMarchingTool.xml
  Interactions/PickingTool.xml
  Interactions/PressMoveRelease.xml
  Interactions/PressMoveReleaseAndPointSetting.xml
  Interactions/PressMoveReleaseWithCTRLInversion.xml
  Interactions/PressMoveReleaseWithCTRLInversionAllMouseMoves.xml
  Interactions/SegmentationToolsConfig.xml

  Interactions/ContourModelModificationConfig.xml
  Interactions/ContourModelModificationInteractor.xml
)

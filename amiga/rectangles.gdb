break pop2RectProbeDone
commands
 silent
 printf "RECTANGLE_COMPLETE calls=%lu guards=%lu\n",g_rectCount,g_rectGuards
 dump binary memory @RUN@/rectangles.bin g_rectRows g_rectRows+1220
 continue
end
break pop2CoreStopped
commands
 silent
 printf "CORE REASON %s\n",g_coreReason
 printf "CORE RESTORE stage=%lu error=%lu vector=%lu heap=%lu zone=%lu dma=%u/%u irq=%u/%u view=%u vbi=%u\n",g_scaffoldStage,g_scaffoldError,g_coreVectorRestored,g_coreHeapOK,g_coreZoneBytes,g_savedDMA,g_actualDMA,g_savedIRQ,g_actualIRQ,g_viewRestored,g_vectorRestored
 continue
end
break pop2ScaffoldRestored
commands
 silent
 echo RECTANGLE RESTORED\n
 detach
 quit 0
end
continue

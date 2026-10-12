break *pop2DisplaySceneComplete
break *pop2CoreStopped
break *pop2DisplayOddFieldComplete
continue
if $pc != (unsigned long)&pop2DisplaySceneComplete || g_displayScene != 1
 printf "DISPLAY SCENE STOP %s trap=%04lx\n",g_coreReason,g_coreStopTrap
 quit 1
end
printf "DISPLAY_SCENE id=1 planes=%08lx copper=%08lx parity=%u line=%u fields=%lu frames=%lu late=%u\n",g_displayPlanes,g_displayCopper,g_displayParity,g_displayLine,g_displayFields,g_displayFrames,g_displayLate
dump binary memory @RUN@/scene-1-planes.bin g_displayPlanes g_displayPlanes+196608
dump binary memory @RUN@/scene-1-copper.bin g_displayCopper g_displayCopper+2236
dump binary memory @RUN@/scene-1-logical.bin g_displayLogicalScreen g_displayLogicalScreen+307200
printf "DISPLAY_EVEN id=1 lof=%u\n",g_displayLOF
continue
if $pc != (unsigned long)&pop2DisplayOddFieldComplete || g_displayParity != 1 || g_displayScene != 1
 echo DISPLAY SCENES / ODD FIELD STOP\n
 quit 1
end
printf "DISPLAY_ODD id=1 planes=%08lx copper=%08lx parity=%u lof=%u fields=%lu frames=%lu late=%u\n",g_displayPlanes,g_displayCopper,g_displayParity,g_displayLOF,g_displayFields,g_displayFrames,g_displayLate
dump binary memory @RUN@/scene-1-odd-copper.bin g_displayCopper g_displayCopper+2236
continue
if $pc != (unsigned long)&pop2DisplaySceneComplete || g_displayScene != 2
 printf "DISPLAY SCENE STOP %s trap=%04lx\n",g_coreReason,g_coreStopTrap
 quit 1
end
printf "DISPLAY_SCENE id=2 planes=%08lx copper=%08lx parity=%u line=%u fields=%lu frames=%lu late=%u\n",g_displayPlanes,g_displayCopper,g_displayParity,g_displayLine,g_displayFields,g_displayFrames,g_displayLate
dump binary memory @RUN@/scene-2-planes.bin g_displayPlanes g_displayPlanes+196608
dump binary memory @RUN@/scene-2-copper.bin g_displayCopper g_displayCopper+2236
dump binary memory @RUN@/scene-2-logical.bin g_displayLogicalScreen g_displayLogicalScreen+307200
printf "DISPLAY_EVEN id=2 lof=%u\n",g_displayLOF
continue
if $pc != (unsigned long)&pop2DisplayOddFieldComplete || g_displayParity != 1 || g_displayScene != 2
 echo DISPLAY SCENES / ODD FIELD STOP\n
 quit 1
end
printf "DISPLAY_ODD id=2 planes=%08lx copper=%08lx parity=%u lof=%u fields=%lu frames=%lu late=%u\n",g_displayPlanes,g_displayCopper,g_displayParity,g_displayLOF,g_displayFields,g_displayFrames,g_displayLate
dump binary memory @RUN@/scene-2-odd-copper.bin g_displayCopper g_displayCopper+2236
continue
if $pc != (unsigned long)&pop2CoreStopped
 echo DISPLAY SCENES / RESTORE STOP\n
 quit 1
end
printf "CORE REASON %s\n",g_coreReason
printf "CORE RESTORE stage=%lu error=%lu vector=%lu heap=%lu zone=%lu dma=%u/%u irq=%u/%u view=%u vbi=%u\n",g_scaffoldStage,g_scaffoldError,g_coreVectorRestored,g_coreHeapOK,g_coreZoneBytes,g_savedDMA,g_actualDMA,g_savedIRQ,g_actualIRQ,g_viewRestored,g_vectorRestored
printf "CORE EXCEPTION vector=%lu pc=%08lx\n",g_coreException,g_coreFaultPC
detach
quit 0

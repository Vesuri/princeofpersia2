break pop2UIOverlayShown
commands
 silent
 printf "INTEGRATION ALERT id=%lu\n",g_uiOverlay
 if g_uiOverlay==7001
  dump binary memory @RUN@/save-picker-screen.bin g_uiOverlayPixels g_uiOverlayPixels+307200
 end
 if g_uiOverlay==7002
  dump binary memory @RUN@/load-picker-screen.bin g_uiOverlayPixels g_uiOverlayPixels+307200
 end
 if g_uiOverlay!=7001 && g_uiOverlay!=7002
  detach
  quit 1
 end
 continue
end
break pop2CoreStopped
commands
 silent
 printf "CORE STOP trap=%04lx segment=%lu offset=%04lx count=%lu\n",g_coreStopTrap,g_coreStopSegment,g_coreStopOffset,g_coreCount
 printf "GAMEPLAY AUDIO selector=%lu menu=%08lx\n",g_gameplayAudioSelector,g_gameplayMenu
 printf "GAMEPLAY SUSPEND calls=%lu/%lu clock=%lu/%lu\n",g_gameplaySuspendCalls,g_gameplayResumeCalls,g_gameplaySuspendClock,g_gameplayResumeClock
 printf "CORE REASON %s\n",g_coreReason
 printf "GAMEPLAY INPUT actions=%lu keys=%lu\n",g_gameplayInputActions,g_gameplayKeyCount
 set $i=0
 while $i<g_gameplayKeyCount && $i<128
  printf "GAMEPLAY KEYS step=%lu left=%lu right=%lu command=%lu\n",g_gameplayKeys[$i][0],g_gameplayKeys[$i][1],g_gameplayKeys[$i][2],g_gameplayKeys[$i][3]
  set $i=$i+1
 end
 printf "CORE RESTORE stage=%lu error=%lu vector=%lu heap=%lu zone=%lu dma=%u/%u irq=%u/%u view=%u vbi=%u\n",g_scaffoldStage,g_scaffoldError,g_coreVectorRestored,g_coreHeapOK,g_coreZoneBytes,g_savedDMA,g_actualDMA,g_savedIRQ,g_actualIRQ,g_viewRestored,g_vectorRestored
 printf "CORE EXCEPTION vector=%lu pc=%08lx\n",g_coreException,g_coreFaultPC
 printf "INTEGRATION INPUT right_reads=%lu\n",g_integrationRightReads
 printf "INTEGRATION SPACE events=%lu\n",g_integrationSpaceEvents
 printf "INTEGRATION ESC reads=%lu\n",g_integrationEscReads
 printf "INTEGRATION AUDIT code=%lu input=%lu\n",g_coreCodeAudit,g_inputVectorRestored
 continue
end
break pop2GameplayFileCheckpoint
commands
 silent
 printf "GAMEPLAY FILE save=%lu load=%lu\n",g_gameplaySaveResult,g_gameplayLoadResult
 set $context=*(unsigned long*)(g_coreA5-0x5014)
 if g_gameplayLoadResult==0
  dump binary memory @RUN@/saved-context.bin $context $context+0x2200
  dump binary memory @RUN@/saved-a5.bin g_coreA5-g_coreBelow g_coreA5
 else
  dump binary memory @RUN@/load-return-context.bin $context $context+0x2200
  dump binary memory @RUN@/load-return-a5.bin g_coreA5-g_coreBelow g_coreA5
 end
 continue
end
break pop2ScaffoldRestored
commands
 silent
 echo INTEGRATION RESTORED\n
 detach
 quit 0
end
break pop2IntegrationStage
commands
 silent
 printf "INTEGRATION STAGE %lu fields=%lu kind=%lu\n",g_integrationStage,g_scaffoldFields,g_integrationDeathKind
 if g_integrationStage==2
  printf "INTEGRATION TITLE phase=%lu cue=%lu\n",g_integrationTitlePhase,g_integrationTitleCue
  dump binary memory @RUN@/title-screen.bin g_integrationScreen g_integrationScreen+307200
  set $colors=*(unsigned long*)g_integrationColors
  dump binary memory @RUN@/title-colors.bin $colors $colors+2056
 end
 if g_integrationStage==3
  dump binary memory @RUN@/first-room-screen.bin g_integrationScreen g_integrationScreen+307200
  set $colors=*(unsigned long*)g_integrationColors
  dump binary memory @RUN@/first-room-colors.bin $colors $colors+2056
  set $context=*(unsigned long*)(g_coreA5-0x5014)
  dump binary memory @RUN@/first-room-context.bin $context $context+0x2200
  dump binary memory @RUN@/first-room-a5.bin g_coreA5-g_coreBelow g_coreA5
  dump binary memory @RUN@/first-room-planes.bin g_displayPlanes g_displayPlanes+196608
  printf "INTEGRATION DISPLAY planes=%08lx copper=%08lx parity=%u frames=%lu\n",g_displayPlanes,g_displayCopper,g_displayParity,g_displayFrames
  dump binary memory @RUN@/first-room-copper.bin g_displayCopper g_displayCopper+2236
 end
 if g_integrationStage==5
  printf "GAMEPLAY SAVE rooms=%lu\n",g_gameplayRooms
  set $context=*(unsigned long*)(g_coreA5-0x5014)
  dump binary memory @RUN@/loaded-first-context.bin $context $context+0x2200
  dump binary memory @RUN@/loaded-first-a5.bin g_coreA5-g_coreBelow g_coreA5
  dump binary memory @RUN@/loaded-first-screen.bin g_integrationScreen g_integrationScreen+307200
  set $colors=*(unsigned long*)g_integrationColors
  dump binary memory @RUN@/loaded-first-colors.bin $colors $colors+2056
  dump binary memory @RUN@/loaded-first-planes.bin g_displayPlanes g_displayPlanes+196608
  printf "GAMEPLAY FIRST DISPLAY planes=%08lx parity=%u\n",g_displayPlanes,g_displayParity
  dump binary memory @RUN@/loaded-first-copper.bin g_displayCopper g_displayCopper+2236
 end
 if g_integrationStage==4
  printf "GAMEPLAY SAVE rooms=%lu\n",g_gameplayRooms
  set $context=*(unsigned long*)(g_coreA5-0x5014)
  dump binary memory @RUN@/loaded-context.bin $context $context+0x2200
  dump binary memory @RUN@/loaded-a5.bin g_coreA5-g_coreBelow g_coreA5
  dump binary memory @RUN@/loaded-screen.bin g_integrationScreen g_integrationScreen+307200
  set $colors=*(unsigned long*)g_integrationColors
  dump binary memory @RUN@/loaded-colors.bin $colors $colors+2056
  dump binary memory @RUN@/loaded-planes.bin g_displayPlanes g_displayPlanes+196608
  printf "GAMEPLAY DISPLAY planes=%08lx parity=%u\n",g_displayPlanes,g_displayParity
  dump binary memory @RUN@/loaded-copper.bin g_displayCopper g_displayCopper+2236
 end
 continue
end
continue

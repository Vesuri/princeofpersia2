break pop2UIOverlayShown
commands
 silent
 printf "INTEGRATION ALERT id=%lu\n",g_uiOverlay
 detach
 quit 1
end
break pop2CoreStopped
commands
 silent
 printf "CORE STOP trap=%04lx segment=%lu offset=%04lx count=%lu\n",g_coreStopTrap,g_coreStopSegment,g_coreStopOffset,g_coreCount
 printf "CORE REASON %s\n",g_coreReason
 printf "CORE RESTORE stage=%lu error=%lu vector=%lu heap=%lu zone=%lu dma=%u/%u irq=%u/%u view=%u vbi=%u\n",g_scaffoldStage,g_scaffoldError,g_coreVectorRestored,g_coreHeapOK,g_coreZoneBytes,g_savedDMA,g_actualDMA,g_savedIRQ,g_actualIRQ,g_viewRestored,g_vectorRestored
 printf "CORE EXCEPTION vector=%lu pc=%08lx\n",g_coreException,g_coreFaultPC
 printf "INTEGRATION INPUT right_reads=%lu\n",g_integrationRightReads
 printf "INTEGRATION SPACE events=%lu\n",g_integrationSpaceEvents
 printf "INTEGRATION ESC reads=%lu\n",g_integrationEscReads
 printf "INTEGRATION AUDIT code=%lu input=%lu\n",g_coreCodeAudit,g_inputVectorRestored
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
 printf "INTEGRATION STAGE %lu fields=%lu right=%lu kind=%lu\n",g_integrationStage,g_scaffoldFields,g_integrationRightAt,g_integrationDeathKind
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
 if g_integrationStage==4
  set $context=*(unsigned long*)(g_coreA5-0x5014)
  dump binary memory @RUN@/death-context.bin $context $context+0x2200
  dump binary memory @RUN@/death-a5.bin g_coreA5-g_coreBelow g_coreA5
  dump binary memory @RUN@/death-screen.bin g_integrationScreen g_integrationScreen+307200
 end
 continue
end
tbreak pop2CoreSegmentLoaded if g_coreLoadedSegment==15
continue
if g_coreLoadedSegment!=15
 echo CUTSCENE INVALID LOAD STOP\n
 detach
 quit 1
end
set $nisbase=g_coreSegments[15][0]
set $nisordinal=0
set $nisselector=0
set $niscopy=0
set $nistext=0
if *(unsigned short*)($nisbase+4)!=20054
 echo CUTSCENE INVALID SITE\n
 detach
 quit 1
end
if *(unsigned short*)($nisbase+358)!=20085
 echo CUTSCENE INVALID SITE\n
 detach
 quit 1
end
if *(unsigned short*)($nisbase+13072)!=20085
 echo CUTSCENE INVALID SITE\n
 detach
 quit 1
end
if *(unsigned short*)($nisbase+1048)!=20085
 echo CUTSCENE INVALID SITE\n
 detach
 quit 1
end
break *($nisbase+4)
commands
 silent
 set $nisordinal=$nisordinal+1
 set $nisselector=*(unsigned short*)($sp+4)
 set $niscopy=0
 set $nistext=0
 printf "CUTSCENE_ENTER ordinal=%u selector=%u\n",$nisordinal,$nisselector
 continue
end
break *($nisbase+13072)
commands
 silent
 set $niscopy=$niscopy+1
 if $niscopy==1 || $niscopy==8 || $niscopy==32 || $niscopy==128 || $niscopy==512
 eval "dump binary memory @RUN@/nis-%u-%u-copy-%u-screen.bin g_integrationScreen g_integrationScreen+307200",$nisordinal,$nisselector,$niscopy
 set $colors=*(unsigned long*)g_integrationColors
 eval "dump binary memory @RUN@/nis-%u-%u-copy-%u-colors.bin $colors $colors+2056",$nisordinal,$nisselector,$niscopy
 eval "dump binary memory @RUN@/nis-%u-%u-copy-%u-a5.bin g_coreA5-g_coreBelow g_coreA5",$nisordinal,$nisselector,$niscopy
 printf "CUTSCENE_FRAME ordinal=%u selector=%u kind=copy count=%u\n",$nisordinal,$nisselector,$niscopy
 end
 continue
end
break *($nisbase+1048)
commands
 silent
 set $nistext=$nistext+1
 if $nistext==1 || $nistext==8 || $nistext==32 || $nistext==128 || $nistext==512
 eval "dump binary memory @RUN@/nis-%u-%u-text-%u-screen.bin g_integrationScreen g_integrationScreen+307200",$nisordinal,$nisselector,$nistext
 set $colors=*(unsigned long*)g_integrationColors
 eval "dump binary memory @RUN@/nis-%u-%u-text-%u-colors.bin $colors $colors+2056",$nisordinal,$nisselector,$nistext
 eval "dump binary memory @RUN@/nis-%u-%u-text-%u-a5.bin g_coreA5-g_coreBelow g_coreA5",$nisordinal,$nisselector,$nistext
 printf "CUTSCENE_FRAME ordinal=%u selector=%u kind=text count=%u\n",$nisordinal,$nisselector,$nistext
 end
 continue
end
break *($nisbase+358)
commands
 silent
 eval "dump binary memory @RUN@/nis-%u-%u-return-%u-screen.bin g_integrationScreen g_integrationScreen+307200",$nisordinal,$nisselector,1
 set $colors=*(unsigned long*)g_integrationColors
 eval "dump binary memory @RUN@/nis-%u-%u-return-%u-colors.bin $colors $colors+2056",$nisordinal,$nisselector,1
 eval "dump binary memory @RUN@/nis-%u-%u-return-%u-a5.bin g_coreA5-g_coreBelow g_coreA5",$nisordinal,$nisselector,1
 printf "CUTSCENE_FRAME ordinal=%u selector=%u kind=return count=%u\n",$nisordinal,$nisselector,1
 printf "CUTSCENE_RETURN ordinal=%u selector=%u result=%u copy=%u text=%u\n",$nisordinal,$nisselector,$d0&65535,$niscopy,$nistext
 continue
end
continue

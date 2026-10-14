set $hallEdit=0
set $hallConfirm=0
break pop2UIOverlayShown
commands
 silent
 if g_uiOverlay==7003
  set $hallEdit=$hallEdit+1
 else
  if g_uiOverlay==7004
   set $hallConfirm=$hallConfirm+1
  else
   printf "HALL UNEXPECTED UI id=%lu\n",g_uiOverlay
  end
 end
 continue
end
break pop2GameplayRouteCheckpoint
commands
 silent
 set $context=*(unsigned long*)(g_coreA5-0x5014)
 printf "ROUTE CHECKPOINT ordinal=%lu frames=%lu keys=%lu level=%u room=%u step=%u planes=%08lx parity=%u\n",g_routeCheckpoint,g_routeFrames,g_routeKeys,*(unsigned short*)(g_coreA5-0x4fd2),*(unsigned short*)(g_coreA5-0x4fc2),*(unsigned short*)(g_coreA5-0x349a),g_displayPlanes,g_displayParity
 eval "dump binary memory @RUN@/route-%u-screen.bin g_integrationScreen g_integrationScreen+307200",g_routeCheckpoint
 eval "dump binary memory @RUN@/route-%u-a5.bin g_coreA5-g_coreBelow g_coreA5",g_routeCheckpoint
 eval "dump binary memory @RUN@/route-%u-context.bin $context $context+0x2200",g_routeCheckpoint
 eval "dump binary memory @RUN@/route-%u-planes.bin g_displayPlanes g_displayPlanes+196608",g_routeCheckpoint
 set $colors=*(unsigned long*)g_integrationColors
 eval "dump binary memory @RUN@/route-%u-colors.bin $colors $colors+2056",g_routeCheckpoint
 eval "dump binary memory @RUN@/route-%u-copper.bin g_displayCopper g_displayCopper+2236",g_routeCheckpoint
 continue
end
break pop2CoreStopped
commands
 silent
 printf "HALL INPUT edit=%u confirm=%u\n",$hallEdit,$hallConfirm
 printf "CORE STOP trap=%04lx segment=%lu offset=%04lx count=%lu\n",g_coreStopTrap,g_coreStopSegment,g_coreStopOffset,g_coreCount
 printf "CORE REASON %s\n",g_coreReason
 printf "ROUTE RESERVE requested=%lu before=%lu after=%lu error=%lu\n",g_routeReserve[0],g_routeReserve[1],g_routeReserve[2],g_routeReserve[3]
 printf "ROUTE RESULT complete=%lu rooms=%lu frames=%lu keys=%lu checkpoints=%lu last=%04lx\n",g_routeComplete,g_routeRooms,g_routeFrames,g_routeKeys,g_routeCheckpoint,g_routeLastResult
 printf "CORE RESTORE stage=%lu error=%lu vector=%lu heap=%lu zone=%lu dma=%u/%u irq=%u/%u view=%u vbi=%u\n",g_scaffoldStage,g_scaffoldError,g_coreVectorRestored,g_coreHeapOK,g_coreZoneBytes,g_savedDMA,g_actualDMA,g_savedIRQ,g_actualIRQ,g_viewRestored,g_vectorRestored
 printf "CORE EXCEPTION vector=%lu pc=%08lx\n",g_coreException,g_coreFaultPC
 printf "INTEGRATION AUDIT code=%lu input=%lu\n",g_coreCodeAudit,g_inputVectorRestored
 continue
end
break pop2ScaffoldRestored
commands
 silent
 echo ROUTE RESTORED\n
 detach
 quit 0
end
continue

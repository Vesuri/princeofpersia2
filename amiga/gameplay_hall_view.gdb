define hook-stop
 printf "HALL OBSERVE phase=%lu requests=%lu pending=%lu\n",g_hallViewPhase,g_hallViewKeyRequests,g_hallViewKeyRequest
end
set $hallComplete=0
break pop2CoreStopped
commands
 silent
 printf "HALL VIEW KEY REQUESTS %lu\n",g_hallViewKeyRequests
 printf "CORE REASON %s\n",g_coreReason
 printf "CORE STOP trap=%04lx segment=%lu offset=%04lx\n",g_coreStopTrap,g_coreStopSegment,g_coreStopOffset
 printf "CORE RESTORE stage=%lu error=%lu vector=%lu heap=%lu zone=%lu dma=%u/%u irq=%u/%u view=%u vbi=%u\n",g_scaffoldStage,g_scaffoldError,g_coreVectorRestored,g_coreHeapOK,g_coreZoneBytes,g_savedDMA,g_actualDMA,g_savedIRQ,g_actualIRQ,g_viewRestored,g_vectorRestored
 printf "INTEGRATION AUDIT code=%lu input=%lu\n",g_coreCodeAudit,g_inputVectorRestored
 continue
end
break pop2ScaffoldRestored
commands
 silent
 echo HALL VIEW RESTORED\n
 detach
 if $hallComplete==1
  quit 0
 end
 quit 1
end
break pop2IntegrationStage
commands
 silent
 printf "HALL STARTUP stage=%lu\n",g_integrationStage
 if g_integrationStage!=3
  continue
 end
end
continue
if g_integrationStage!=3
 echo HALL VIEW INVALID STARTUP STOP\n
 detach
 quit 1
end
delete 3
set $base=g_coreSegments[2][0]
tbreak *($base+0x20f4)
continue
if $pc!=$base+0x20f4
 echo HALL VIEW INVALID ORIGINAL STOP\n
 detach
 quit 1
end
set $table=*(unsigned long*)($sp+4)
if *(unsigned short*)$table!=1
 echo HALL VIEW INVALID TABLE COUNT\n
 detach
 quit 1
end
set $size=2+30*(*(unsigned short*)$table)
printf "HALL VIEW TABLE count=%u\n",*(unsigned short*)$table
dump binary memory @RUN@/hall-view-table.bin $table $table+$size
tbreak pop2UIOverlayShown
continue
if g_uiOverlay!=7005
 echo HALL VIEW INVALID OVERLAY\n
 detach
 quit 1
end
printf "HALL VIEW SHOWN id=%lu\n",g_uiOverlay
dump binary memory @RUN@/hall-ui-screen.bin g_uiOverlayPixels g_uiOverlayPixels+307200
tbreak *($base+0x291e)
continue
if $pc!=$base+0x291e
 echo HALL VIEW INVALID FADE STOP\n
 detach
 quit 1
end
echo HALL VIEW FADED IN\n
tbreak *($base+0x29ca)
continue
if $pc!=$base+0x29ca
 echo HALL VIEW INVALID ORIGINAL STOP\n
 detach
 quit 1
end
echo HALL VIEW RETURN\n
tbreak *($base+0x5912)
continue
if $pc!=$base+0x5912
 echo HALL VIEW INVALID ORIGINAL STOP\n
 detach
 quit 1
end
echo HALL VIEW PAUSED\n
tbreak *($base+0x68b2)
continue
if $pc!=$base+0x68b2
 echo HALL VIEW INVALID ORIGINAL STOP\n
 detach
 quit 1
end
echo HALL VIEW RESUMED\n
set $hallComplete=1
continue

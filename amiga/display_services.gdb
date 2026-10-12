break *pop2DisplayServicesComplete
break *pop2CoreStopped
continue
if $pc != (unsigned long)&pop2DisplayServicesComplete
 printf "DISPLAY SERVICE STOP %s\n",g_coreReason
 quit 1
end
set $p=g_displayServiceProbe
printf "DISPLAY SERVICES probe=%08lx device=%08lx port=%08lx map=%08lx result=%u lock=%u base=%08lx unlocked=%08lx\n",$p,*(unsigned long*)($p+212),*(unsigned long*)($p+216),*(unsigned long*)($p+220),*(unsigned short*)($p+232),*(unsigned short*)($p+234),*(unsigned long*)($p+236),*(unsigned long*)($p+240)
dump binary memory @RUN@/display-service-probe.bin $p $p+244
set $port=*(unsigned long*)($p+216)
dump binary memory @RUN@/display-service-port.bin $port $port+108
set $map=*(unsigned long*)(*(unsigned long*)($p+220))
dump binary memory @RUN@/display-service-map.bin $map $map+50
set $device=*(unsigned long*)(*(unsigned long*)($p+212))
dump binary memory @RUN@/display-service-device.bin $device $device+62
set $screen=*(unsigned long*)(*(unsigned long*)($device+22))
dump binary memory @RUN@/display-service-screen.bin $screen $screen+50
continue
if $pc != (unsigned long)&pop2CoreStopped
 echo DISPLAY SERVICES / RESTORE STOP\n
 quit 1
end
printf "CORE REASON %s\n",g_coreReason
printf "CORE RESTORE stage=%lu error=%lu vector=%lu heap=%lu zone=%lu dma=%u/%u irq=%u/%u view=%u vbi=%u\n",g_scaffoldStage,g_scaffoldError,g_coreVectorRestored,g_coreHeapOK,g_coreZoneBytes,g_savedDMA,g_actualDMA,g_savedIRQ,g_actualIRQ,g_viewRestored,g_vectorRestored
printf "CORE EXCEPTION vector=%lu pc=%08lx\n",g_coreException,g_coreFaultPC
detach
quit 0

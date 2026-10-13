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
 printf "CLOCK FAULT %lu\n",g_clockFault
 printf "CORE RESTORE stage=%lu error=%lu vector=%lu heap=%lu zone=%lu dma=%u/%u irq=%u/%u view=%u vbi=%u\n",g_scaffoldStage,g_scaffoldError,g_coreVectorRestored,g_coreHeapOK,g_coreZoneBytes,g_savedDMA,g_actualDMA,g_savedIRQ,g_actualIRQ,g_viewRestored,g_vectorRestored
 printf "CORE EXCEPTION vector=%lu pc=%08lx\n",g_coreException,g_coreFaultPC
 printf "TIMING VBL result=%lu elapsed=%lu user_calls=%lu deferred_fields=%lu\n",g_timingVblResult,g_timingVblElapsed,g_timingVblUserCalls,g_timingVblDeferredFields
 dump binary memory @RUN@/timing-vbl.bin g_timingVblRecords g_timingVblRecords+160
 printf "INTEGRATION INPUT right_reads=%lu\n",g_integrationRightReads
 printf "INTEGRATION SPACE events=%lu\n",g_integrationSpaceEvents
 printf "INTEGRATION ESC reads=%lu\n",g_integrationEscReads
 printf "INTEGRATION AUDIT code=%lu input=%lu\n",g_coreCodeAudit,g_inputVectorRestored
 continue
end
break pop2ScaffoldRestored
commands
 silent
 echo TIMING VBL RESTORED\n
 detach
 quit 0
end
continue

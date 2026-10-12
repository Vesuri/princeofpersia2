break *pop2ScaffoldRestored
continue
if $pc != (unsigned long)&pop2ScaffoldRestored
 echo SCAFFOLD / WRONG STOP\n
 quit 1
end
if g_scaffoldStage != 3 || g_scaffoldError != 0 || g_scaffoldFields < 4
 echo SCAFFOLD / INCOMPLETE\n
 quit 1
end
if g_savedDMA != g_actualDMA || g_savedIRQ != g_actualIRQ || g_viewRestored != 1 || g_vectorRestored != 1
 echo SCAFFOLD / RESTORE MISMATCH\n
 quit 1
end
printf "SCAFFOLD PROBE PASS stage=%lu fields=%lu dma=%u irq=%u view=%u vector=%u\n", g_scaffoldStage, g_scaffoldFields, g_actualDMA, g_actualIRQ, g_viewRestored, g_vectorRestored
detach
quit 0

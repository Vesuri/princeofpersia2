"""Check the effective native machine against an explicitly selected preset."""

def check(run,setup="a1200-fast"):
    config=dict(line.split('=',1) for line in (run/'logs/debug.uae').read_text().splitlines() if '=' in line)
    expected={'cpu_model':'68020','chipset':'aga','chipmem_size':'4','fastmem_size':'8','ntsc':'false','cpu_speed':'real','cachesize':'0','cpu_frequency':'14187580','cpu_data_cache':'false'}
    if setup=='a1200-030':
        expected.update(cpu_model='68030',mmu_model='68030',cpu_frequency='50000000',cpu_cycle_exact='true',cpu_memory_cycle_exact='true',cpu_data_cache='true')
    elif setup=='a1200-040':
        expected.pop('cpu_frequency')
        expected.update(cpu_model='68040',cpu_multiplier='8',mmu_model='68040',fpu_model='68040',cpu_cycle_exact='false',cpu_data_cache='true')
    else:
        if setup!='a1200-fast':raise RuntimeError('NATIVE / UNSUPPORTED MACHINE CHECK')
    if any(config.get(k)!=v for k,v in expected.items()):raise RuntimeError('NATIVE / EFFECTIVE MACHINE MISMATCH')

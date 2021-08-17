# Arbitrary Physical Memory + Control Register Read Write

So I found this driver in my downloads folder, and decided to give it a look in ida. Turns out this driver is very vulnerable as it provides full physical memory r/w, control register modification, pci read write, allocate + deallocate physical memory and more. I only wrote this PoC in 30 minutes and it has important functions like phys r/w, control register + msr r/w, you can take a look at this driver and abuse it to manual map your own driver and more.


This driver is signed and is probably unchecked by EAC, BE and Vanguard. Might be wrong tho.

# Development notes: Quick Volume

Note: The addresses and offsets used are from firmware 3.65.

The "Master Volume" slidebar is enabled by default for the PSTV so all that has to be done is to enable it for the Vita and redirect usages of master volume to system volume.

#### Tools

- [Ghidra](https://ghidra-sre.org)
- [ARMv7 reference manual](https://static.docs.arm.com/ddi0406/cd/DDI0406C_d_armv7ar_arm.pdf)
- [CXML-Decompiler](https://bitbucket.org/SilicaAndPina/cxml-decompiler)
- [Decrypted firmware modules](https://github.com/KuromeSan/psvita-elfs)


#### Enabling the volume slidebar

The slidebar itself is enabled by injecting `cmp r0, r0` at `0x8114D074`:

```C
tai_module_info_t minfo;
minfo.size = sizeof(minfo);
taiGetModuleInfo("SceShell", &minfo);
taiInjectData(minfo.modid, 0, 0x14D074, "\x80\x42", 2);
````

Here is the assembly at this location:

```
8114d070 0e f3 0a ec     blx        ScePafMisc_7ED8F03C
8114d074 00 28           cmp        r0,#0x0
8114d076 04 d1           bne        LAB_8114d082
8114d078 17 f1 a4 00     adds.w     r0,r7,#0xa4
8114d07c 21 1c           add        r1,r4,#0x0
8114d07e 05 f0 a1 ff     bl         FUN_81152fc4
```

The effect of the injection is that the call to `FUN_81152fc4` is unconditional. This is in fact the function to create the slidebar.

Take a look at this snippet from the body of `FUN_81152fc4`.

```
81153016 4f f2 3a 30     movw       r0,#0xf33a
8115301a c8 f2 e0 40     movt       r0,#0x84e0
8115301e 0a 90           str        r0,[sp,#0x28]
81153020 05 a8           add        r0,sp,#0x14
81153022 09 f3 da ee     blx        ScePafToplevel_99300D8B
```

`ScePafToplevel_99300D8B` is called with `r0` equal to `sp+0x14`, which is a pointer to a struct on the stack. At `sp+0x28` we have the value `0x84e0f33a`, which is the `id` of a `template` tag in the main XML of `vs0:vsh/shell/impose_plugin.rco`. Under this `template` is a `text` tag with `label=33FCE3F2`. In the English strings XML, you can find that `33FCE3F2` corresponds to the string `Master Volume`.


#### Handling AVLS in the volume slidebar callback

For `sceAVConfigGetMasterVol` and `sceAVConfigWriteMasterVol`, we have simply hooked these as imports of `SceShell` and called the system volume counterparts in the hook functions, but for setting the volume, we have to do a little more to handle AVLS. Recall that some games will limit the maximum brightness and the brightness slidebar will refuse to stay at the maximum. That is exactly the behaviour we want for when AVLS is enabled.

In the body of `FUN_81152fc4`, we can see a `0x14` byte struct with a field set to the address of `FUN_811535e6` and the struct being passed into `ScePafWidget_FB7FE189`. Note that the actual value set is `0x811535e7` because this function runs in thumb mode. `FUN_811535e6` is the slidebar callback for the master volume slidebar. This is the decompiler output:

```
void FUN_811535e6(int r0) {
	sceAVConfigSetMasterVol(*(int*)(*(int*)(r0 + 0x8) + 0x294));
}
```

From this we can see that the slidebar position resides at `*(int*)(r0 + 0x8) + 0x294`. Similarly, we can find the callback function for the brightness slidebar:

```
void FUN_8114de18(int r0, int r1, int r2, int r3) {
	uint var1;
	uint var2;

	if (*(int*)(r3 + 0x8) != 0x0) {
		var1 = *(int*)(*(int*)(r3 + 0x8) + 0x294) + 0x15;
		sceAVConfigGetDisplayMaxBrightness(&var2);
		if (var2 < var1) {
			(**(code**)(**(int**)(r3 + 0x8) + 0x188))(*(int**)(r3 + 0x8), &DAT_ffffffeb + var2, 0x0);
			var1 = var2;
		}
		sceAVConfigSetDisplayBrightness(var1);
	}
}
```

Instead of `r0` it is `r3` here but it makes no difference for our purpose. From this we can see that

1. `*(int*)(r3 + 0x8)` is a pointer to an object.
2. `**(int**)(r3 + 0x8)` is the pointer to the object's vtable.
3. `**(code**)(**(int**)(r3 + 0x8) + 0x188)` is the function that sets the slidebar position to the value of the second argument.

With these facts we can hook and reimplement `FUN_811535e6` to handle AVLS.

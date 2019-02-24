[bits 32]

;void* memset(void* ptr, int value, size_t num)
global memset
memset:
	push edi	
	mov ecx, 	[esp + 16]
	mov al, 	[esp + 12]
	mov edi, 	[esp + 8]
	cld
	rep stosb
	mov eax, 	[esp + 8]
	pop edi
	ret
	
;void* memcpy(void* dest, const void* src, size_t num) 
global memcpy
memcpy:
	push edi
	push esi
	mov ecx, 	[esp + 20]
	mov esi, 	[esp + 16]
	mov edi, 	[esp + 12]
	mov eax, ecx
	shr ecx, 2		;convert bytes to dwords		
	and eax, 3		;whats the remainder?
	cld
	rep movsd		;copy as many dwords as possible
	mov ecx, eax	
	rep movsb		;copy the rest of the bytes individually
	mov eax, 	[esp + 12]
	pop esi
	pop edi
	ret
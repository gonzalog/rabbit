# Trabajo práctico de Criptografía

## Para compilar

`gcc -o rabbit rabbit.c`

## Ejemplo de ejecución

Los parámetros son: 
 - Clave de 16 caracteres
 - Path al archivo a cifrar/descifrar
 - Path de salida
 - Tamaño del header que se desee mantener (puede ser 0)
 - Valor inicial (opcional)

### Sin valor inicial
`./rabbit 128BitsSecretKey ./rabbit.bmp ./crypto_rabbit.bmp 54`

### Con valor inicial
`./rabbit 128BitsSecretKey ./rabbit.bmp ./crypto_rabbit.bmp 54 init_val`

## Ejemplo
### Imagen original
![original](http://i.imgur.com/zeH6S2H.png)
### Imagen cifrada
![cifrada](http://i.imgur.com/Q9GawTi.png)
### Imagen descifrada
![descifrada](http://i.imgur.com/xghMLGI.png)

# Maze

Ez a projekt egy SDL2 + OpenGL alapú, első személyű 3D labirintusjáték.

## Program leírása

A játékos egy előre megadott labirintusban mozog, falak között navigálva. A pálya 3D nézetben jelenik meg, textúrázott falakkal, padlóval és égbolttal. A látványt kézi fényerő-szabályzás és kamera-billenés egészíti ki, így a mozgás természetesebb hatást kelt.

A program a következőket használja:

- SDL2 ablakkezeléshez és eseménykezeléshez
- OpenGL a 3D rendereléshez
- SDL_image textúrák betöltéséhez
- Egy OBJ modell betöltését a jelenetben megjelenő piknikasztalhoz

## Játékmenet

- A játékos egy fix kezdőpontról indul a labirintusban.
- A falak ütközéssel rendelkeznek, ezért nem lehet átmenni rajtuk.
- A jelenetben egy asztalmodell is látható, ami a pálya részeként viselkedik.
- Az F1 billentyűvel használati útmutató jeleníthető meg a képernyőn.

## Vezérlés

- `W`, `A`, `S`, `D`: mozgás
- Egér mozgatása: nézelődés
- `F1`: segítség fel- és lekapcsolása
- Fel nyíl: fényerő növelése
- Le nyíl: fényerő csökkentése
- `ESC`: kilépés

## Követelmények

- Windows környezet
- SDL2
- SDL2_image
- OpenGL és GLU
- A projektben mellékelt MinGW SDK

## Fordítás

A projektben található egy Makefile, amely a mellékelt MinGW SDK-t használja.

```bash
make
```

Ez létrehozza a `maze.exe` futtatható állományt.

## Futtatás

```bash
./maze.exe
```

Windows alatt a build után közvetlenül elindítható a létrejött `maze.exe` fájl is.

## Fájlstruktúra

- `src/main.c`: a játék fő logikája, renderelés, vezérlés és UI
- `src/config.c`: pályaadatok, kezdő- és célpozíciók, textúraútvonalak
- `assets/`: textúrák és 3D modellfájlok

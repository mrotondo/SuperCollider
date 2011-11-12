QAlignment {
  classvar dict;

  *initClass {
    dict = IdentityDictionary.new;
    dict.put( \left, 16r1 | 16r80 );
    dict.put( \center, 16r4 | 16r80 );
    dict.put( \right, 16r2 | 16r80 );
    dict.put( \topLeft, 16r1 | 16r20 );
    dict.put( \top, 16r4 | 16r20 );
    dict.put( \topRight, 16r2 | 16r20 );
    dict.put( \bottomLeft, 16r1 | 16r40 );
    dict.put( \bottom, 16r4 | 16r40 );
    dict.put( \bottomRight, 16r2 | 16r40 );
  }

  *new { arg alignment; ^dict[alignment]; }
}

QOrientation {
  classvar dict;

  *initClass {
    dict = IdentityDictionary.new;
    dict.put( \horizontal, 1 );
    dict.put( \vertical, 2 );
  }

  *new { arg alignment; ^dict[alignment]; }
}

QLimits {
  classvar dict;

  *initClass {
    dict = IdentityDictionary.new;
    dict.put( \maxWidgetSize, 16777215 );
  }

  *new { arg limit; ^dict[limit]; }
}

QKey {
  classvar
    <left = 16r1000012,
    <up = 16r1000013,
    <right = 16r1000014,
    <down = 16r1000015;
}

QKeyModifiers {
  classvar
    <shift = 16r2000000,
    <control = 16r4000000,
    <alt = 16r8000000,
    <meta = 16r10000000,
    <keypad = 16r20000000;

  *toCocoa { arg mods;
    var cmods = 0;
    if (mods & QKeyModifiers.shift > 0) {cmods = cmods | 131072};
    if (mods & QKeyModifiers.alt > 0 ) {cmods = cmods | 524288};
    Platform.case (
      \osx,
          {
            if (mods & QKeyModifiers.control > 0) {cmods = cmods | 1048576}; // Cmd
            if (mods & QKeyModifiers.meta > 0) {cmods = cmods | 262144}; // Ctrl
          },
      { if (mods & QKeyModifiers.control > 0) {cmods = cmods | 262144} } // Ctrl
    );
    if (mods & QKeyModifiers.keypad > 0) {cmods = cmods | 2097152};
    // TODO: caps-lock, func, help
    ^cmods;
  }
}

QWebFontFamily {
  classvar
    <standard = 0,
    <fixed = 1,
    <serif = 2,
    <sansSerif = 3,
    <cursive = 4,
    <fantasy = 5;

  *new { arg symbol; ^this.perform( symbol.asGetter ); }
}

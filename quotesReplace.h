#pragma once

namespace QUOTES {
  enum Direction {
    encode,
    decode
  };
  char * replace(char * value, QUOTES::Direction direction=encode){
    char from, to;
    // switch (direction) {
    //   case QUOTES::encode:
    //     from='"'; to = '`';
    //     break;
    //   case QUOTES::decode:
    //     from='`'; to = '"';      
    //     break;
    // }
    if( direction == QUOTES::encode ) {
      from='"'; to = '`';
    } else {
      from='`'; to = '"';
    }
    
    uint i =0;
    while( value[i] ){
      if ( value[i] == from ){
        value[i] = to;
      } 
      i++;
    }
    return value;
  };

};
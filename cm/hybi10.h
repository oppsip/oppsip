/**
 * ,.   ,   ,.   .   .---.         .       .  .---.                      
 * `|  /|  / ,-. |-. \___  ,-. ,-. | , ,-. |- \___  ,-. ,-. .  , ,-. ,-. 
 *  | / | /  |-' | |     \ | | |   |<  |-' |      \ |-' |   | /  |-' |   
 *  `'  `'   `-' ^-' `---' `-' `-' ' ` `-' `' `---' `-' '   `'   `-' ' 
 * 
 * Copyright 2012 by Alexander Thiemann <mail@agrafix.net>
 *
 * This file is part of WebSocketServer.
 *
 *  WebSocketServer is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  WebSocketServer is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with WebSocketServer.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#include <stdio.h>
#include <string>
#include <sstream>
#include <vector>

#include <stdlib.h>
#include <math.h>

#ifndef HYBI10_H
#define	HYBI10_H

namespace hybi10 {
    struct request {
        int exitcode;
        std::string type;
        std::string payload;
    };
    
    std::string encode2(request hRequest, bool masked);
    std::string encode(request hRequest, bool masked);
    
    request decode(unsigned char *data,int dataLength);
}

#endif	/* HYBI10_H */


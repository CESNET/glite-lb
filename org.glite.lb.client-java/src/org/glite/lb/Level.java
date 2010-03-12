/*
Copyright (c) Members of the EGEE Collaboration. 2004-2010.
See http://www.eu-egee.org/partners for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

package org.glite.lb;

/**
 * This class represents logging level for events.
 *
 * @author Tomas Kramec, 207545@mail.muni.cz
 */
public class Level {

    private final int level;

    public static final Level LEVEL_UNDEFINED = new Level(0);
    public static final Level LEVEL_EMERGENCY = new Level(1);
    public static final Level LEVEL_ALERT = new Level(2);
    public static final Level LEVEL_ERROR = new Level(3);
    public static final Level LEVEL_WARNING = new Level(4);
    public static final Level LEVEL_AUTH = new Level(5);
    public static final Level LEVEL_SECURITY = new Level(6);
    public static final Level LEVEL_USAGE = new Level(7);
    public static final Level LEVEL_SYSTEM = new Level(8);
    public static final Level LEVEL_IMPORTANT = new Level(9);
    public static final Level LEVEL_DEBUG = new Level(10);

    private Level(int level) {
	this.level = level;
    }

    public int getInt() {
        return level;
    }
    @Override
    public String toString() {
        switch (level) {
            case 0: return "UNDEFINED";
            case 1: return "EMERGENCY";
            case 2: return "ALERT";
            case 3: return "ERROR";
            case 4: return "WARNING";
            case 5: return "AUTH";
            case 6: return "SECURITY";
            case 7: return "USAGE";
            case 8: return "SYSTEM";
            case 9: return "IMPORTANT";
            case 10: return "DEBUG";
            default: throw new IllegalArgumentException("wrong level type");
        }
    }

}

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
 * This class represents the timestamp in this form: <i>tvSec.tvUsec</i>.
 * It consists of two parts:
 * <ol>
 * <li><b>tvSec</b> - represents time in seconds </li>
 * <li><b>tvUsec</b> - represents time in microseconds </li>
 * </ol>
 *
 * For example: 1240415967.234999
 * @author Tomas Kramec, 207545@mail.muni.cz
 */
public class Timeval {

    private long tvSec;
    private long tvUsec;

    public Timeval() {
    }

    /**
     * Creates an instance of Timeval.
     *
     * @param tvSec in seconds
     * @param tvUsec in microseconds
     */
    public Timeval(long tvSec, long tvUsec) {
        this.tvSec = tvSec;
        this.tvUsec = tvUsec;
    }


    /**
     * Gets the tvSec value for this Timeval.
     *
     * @return tvSec in seconds
     */
    public long getTvSec() {
        return tvSec;
    }


    /**
     * Sets the tvSec value for this Timeval.
     *
     * @param tvSec in seconds
     */
    public void setTvSec(long tvSec) {
        this.tvSec = tvSec;
    }


    /**
     * Gets the tvUsec value for this Timeval.
     *
     * @return tvUsec in microseconds
     */
    public long getTvUsec() {
        return tvUsec;
    }


    /**
     * Sets the tvUsec value for this Timeval.
     *
     * @param tvUsec in microseconds
     */
    public void setTvUsec(long tvUsec) {
        this.tvUsec = tvUsec;
    }
}

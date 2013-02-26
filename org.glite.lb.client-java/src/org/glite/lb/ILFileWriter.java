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

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.channels.FileChannel;
import java.nio.channels.FileLock;

/**
 * This class provides writing messages to some file.
 * 
 * @author Pavel Piskac (173297@mail.muni.cz)
 */
public class ILFileWriter {

    public ILFileWriter() {
    }
    
    /**
     * Writes message to a file and returns size of this file before writing the
     * data
     * @param prefix file path
     * @param message message which will be written
     * @param repeatWriteToFile count of attempts to write to file in case of failure
     */
    public static Long write(String prefix, String message, int repeatWriteToFile) throws LBException {
        FileWriter fileWriter = null;
        long fileLength = 0;
        FileLock fileLock = null;
        File file;
	Runtime run = Runtime.getRuntime();
	String cmd[] = {"chmod", "g+rw", null};

        for (int i = 0; i < repeatWriteToFile; i++) {
            try {
                file = new File(prefix);
		FileOutputStream out = new FileOutputStream(file,true);
                FileChannel fileChannel = out.getChannel();

                fileLock = fileChannel.tryLock();
                if (fileLock != null) {
                    if (!file.exists()) {
                        continue;
                    }
                    fileLength = file.length();
		    out.write(message.getBytes());
		    fileLock.release();
		    out.close();

                    if (file.exists()) {
			cmd[2] = prefix;
			run.exec(cmd);
                        break;
                    }
                }
            } catch (Throwable ex) {
                if (fileLock != null) {
                    try {
                        fileLock.release();
                    } catch (IOException ex2) { }
                }
		throw new LBException(ex);
            }
        }

        return new Long(fileLength);
    }
}

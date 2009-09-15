package org.glite.lb;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;
import java.io.RandomAccessFile;
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
    public static Long write(String prefix, String message, int repeatWriteToFile) {
        FileWriter fileWriter = null;
        Long fileLength = null;
        RandomAccessFile raf = null;
        FileLock fileLock = null;
        File file;

        for (int i = 0; i < repeatWriteToFile; i++) {
            try {
                file = new File(prefix);
                raf = new RandomAccessFile(file, "rw");
                FileChannel fileChannel = raf.getChannel();

                fileLock = fileChannel.tryLock();
                if (fileLock != null) {
                    if (!file.exists()) {
                        continue;
                    }
                    fileLength = new Long(raf.length());
                    fileWriter = new FileWriter(file, true);
                    //true means append data at the end of file

                    BufferedWriter bufferedFileWriter = new BufferedWriter(fileWriter);

                    bufferedFileWriter.write(message);
                    bufferedFileWriter.flush();

                    if (file.exists()) {
                        break;
                    }
                }
            } catch (FileNotFoundException ex) {
                System.err.println(ex);
            } catch (IOException ex) {
                System.err.println(ex);
            } catch (Exception ex) {
                System.err.println(ex);
            } finally {
                if (fileLock != null) {
                    try {
                        fileLock.release();
                    } catch (IOException ex) {
                        System.err.println(ex);
                    }
                }

                try {
                    raf.close();
                } catch (IOException ex) {
                    System.err.println(ex);
                } catch (NullPointerException ex) {
                    System.err.println(ex);
                }
            }
        }

        return fileLength;
    }
}

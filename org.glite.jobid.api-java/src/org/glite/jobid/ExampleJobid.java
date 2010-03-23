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

package org.glite.jobid;

/**
 * This class shows how Jobid works and how to work with it.
 * @author Pavel Piskac
 */
public class ExampleJobid {
    
    public static void main(String[] args) {
        //how Jobid class works
        //unique part is automatically generated
        Jobid jobid1 = new Jobid("https://somewhere.cz", 5000);
        System.out.println("bkserver "+ jobid1.getBkserver());
        System.out.println("port "+ jobid1.getPort());
        System.out.println("unique "+ jobid1.getUnique());
        System.out.println("-------------------");
        
        //unique part is set by user
        Jobid jobid2 = new Jobid("https://somewhere.cz", 5000, "my_unique_part");
        System.out.println("bkserver "+ jobid2.getBkserver());
        System.out.println("port "+ jobid2.getPort());
        System.out.println("unique "+ jobid2.getUnique());
        System.out.println("-------------------");
        
        //whole jobid is set by user and then parsed
        Jobid jobid3 = new Jobid("https://somewhere.cz:5000/my_unique_part");
        System.out.println("bkserver "+ jobid3.getBkserver());
        System.out.println("port "+ jobid3.getPort());
        System.out.println("unique "+ jobid3.getUnique());
        System.out.println("-------------------");
        
        //each part is set separately
        Jobid jobid4 = new Jobid();
        jobid4.setBkserver("https://somewhere.cz");
        jobid4.setPort(5000);
        jobid4.setUnique("my_unique_part");
        System.out.println("bkserver "+ jobid4.getBkserver());
        System.out.println("port "+ jobid4.getPort());
        System.out.println("unique "+ jobid4.getUnique());
        System.out.println("-------------------");
    } 

}

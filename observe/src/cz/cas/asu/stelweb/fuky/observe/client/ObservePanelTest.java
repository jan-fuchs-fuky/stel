/*
 *
 *   Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *   $Date$
 *   $Rev$
 *   $URL$
 *
 *   Copyright (C) 2010-2012 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
 *
 *   This file is part of Observe (Observing System for Ondrejov).
 *
 *   Observe is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Observe is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Observe.  If not, see <http://www.gnu.org/licenses/>.
 */

package cz.cas.asu.stelweb.fuky.observe.client;

import static org.junit.Assert.*;

import java.util.LinkedList;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.junit.Test;

public class ObservePanelTest {

    public void checkRegex(Pattern pattern, DataRegex dataRegex) {
        boolean hasNext = true;
        
        System.out.println("checkRegex: " + pattern.pattern());
        
        while (hasNext) {
            String input = dataRegex.getInput();
            
            System.out.format("    checking %11s", input);
            Matcher matcher = pattern.matcher(input);
            
            assertEquals(true, matcher.find());
            
            StringBuffer str = new StringBuffer();

            assertEquals(dataRegex.getResultSize(), matcher.groupCount());

            for (int i = 1; i <= matcher.groupCount(); ++i) {
                assertEquals(dataRegex.getResult(i-1), matcher.group(i));
                str.append(" | " + matcher.group(i));
            }

            System.out.printf("%s OK\n", str.toString());
            hasNext = dataRegex.next();
        }        
    }
    
    class DataRegex {
        private LinkedList<String> inputs = new LinkedList<String>();
        private LinkedList<String[]> results = new LinkedList<String[]>();
        private int index = 0;
        
        public DataRegex() {
        }
        
        public void add(String input, String[] result) {
            inputs.add(input);
            //results.add(ArrayUtils.toObject(result));
            results.add(result);
        }
        
        public int size() {
            return inputs.size();
        }
        
        public boolean next() {
            if (index >= (inputs.size() - 1)) {
                return false;
            }
            
            ++index;
            return true;
        }

        public String getInput() {
            return inputs.get(index);
        }        
        
        public String getResult(int result_idx) {
            String[] result = results.get(index);
            
            return result[result_idx];
        }
        
        public int getResultSize() {
            String[] result = results.get(index);
            
            return result.length;
        }
    }
    
    @Test
    public void testRegex() {
        DataRegex dataRegex = new DataRegex();
        dataRegex.add("1:2:3", new String[]{"1", "2", "3"});
        dataRegex.add("11:2:3", new String[]{"11", "2", "3"});
        dataRegex.add("11:22:3", new String[]{"11", "22", "3"});
        dataRegex.add("11:22:33", new String[]{"11", "22", "33"});
        dataRegex.add("1:2:33", new String[]{"1", "2", "33"});
        dataRegex.add("1:22:33", new String[]{"1", "22", "33"});
        dataRegex.add("11:2:33", new String[]{"11", "2", "33"});
        checkRegex(ObservePanel.PATTERN_H_M_S, dataRegex);
        
        dataRegex = new DataRegex();
        dataRegex.add("2:3", new String[]{"2", "3"});
        dataRegex.add("22:3", new String[]{"22", "3"});
        dataRegex.add("22:33", new String[]{"22", "33"});
        dataRegex.add("2:33", new String[]{"2", "33"});        
        checkRegex(ObservePanel.PATTERN_M_S, dataRegex);
        
        dataRegex = new DataRegex();
        dataRegex.add("1", new String[]{"1"});
        dataRegex.add("12", new String[]{"12"});
        dataRegex.add("123", new String[]{"123"});
        dataRegex.add("1234", new String[]{"1234"});
        dataRegex.add("12345", new String[]{"12345"});
        checkRegex(ObservePanel.PATTERN_S, dataRegex);
        
        dataRegex = new DataRegex();
        dataRegex.add("0.0001", new String[]{"0.0001"});
        dataRegex.add("0.5", new String[]{"0.5"});
        dataRegex.add("1", new String[]{"1"});
        dataRegex.add("1.5", new String[]{"1.5"});
// DEPRECATED
//        dataRegex.add("2k", new String[]{"2", "k"});
//        dataRegex.add("3K", new String[]{"3", "K"});
//        dataRegex.add("4m", new String[]{"4", "m"});
//        dataRegex.add("5M", new String[]{"5", "M"});
//        dataRegex.add("10M", new String[]{"10", "M"});
//        dataRegex.add("100M", new String[]{"100", "M"});
//        dataRegex.add("1000M", new String[]{"1000", "M"});
        checkRegex(ObservePanel.PATTERN_COUNT, dataRegex);
    }
    
    @Test
    public void testParseExposureTime() throws Exception {
        LinkedList<Object[]> data = new LinkedList<Object[]>();
        
        data.add(new Object[]{"02:01:40", new Integer(2 * 3600 + 1 * 60 + 40)});
        data.add(new Object[]{"2:1:40", new Integer(2 * 3600 + 1 * 60 + 40)});

        System.out.println("parseExposureTime():");

        for (Object[] item : data) {
            int result = ObservePanel.parseExposureTime((String)item[0]);

            System.out.printf("%s %s ", item[0], result);
            
            assertEquals(((Integer)item[1]).intValue(), result);
            
            System.out.printf("OK\n");
        }
    }
    
    @Test
    public void testParseExposureMeter() throws Exception {
        LinkedList<Object[]> data = new LinkedList<Object[]>();
        
        data.add(new Object[]{"0.0001", new Integer(100)});
        data.add(new Object[]{"1.5", new Integer(1500000)});
        data.add(new Object[]{"2", new Integer(2000000)});
// DEPRECATED
//        data.add(new Object[]{"200k", new Integer(1000 * 200)});
//        data.add(new Object[]{"15M", new Integer(1000 * 1000 * 15)});

        System.out.println("parseExposureMeter():");

        for (Object[] item : data) {
            int result = ObservePanel.parseExposureMeter((String)item[0]);

            System.out.printf("%s %s ", item[0], result);
            
            assertEquals(((Integer)item[1]).intValue(), result);
            
            System.out.printf("OK\n");
        }
    }
}

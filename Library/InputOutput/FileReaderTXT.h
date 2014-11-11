/* 
 *
 * Copyright (C) 2006-2014 Jedox AG
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License (Version 2) as published
 * by the Free Software Foundation at http://www.gnu.org/copyleft/gpl.html.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * If you are developing and distributing open source applications under the
 * GPL License, then you are free to use Palo under the GPL License.  For OEMs,
 * ISVs, and VARs who distribute Palo with their products, and do not license
 * and distribute their source code under the GPL, Jedox provides a flexible
 * OEM Commercial License.
 *
 * \author Frank Celler, triagens GmbH, Cologne, Germany
 * \author Achim Brandt, triagens GmbH, Cologne, Germany
 * \author Jiri Junek, qBicon s.r.o., Prague, Czech Republic
 * \author Martin Jakl, qBicon s.r.o., Prague, Czech Republic
 * 
 *
 */

#ifndef INPUT_OUTPUT_FILE_READER_TXT_H
#define INPUT_OUTPUT_FILE_READER_TXT_H 1

#include "palo.h"

#include "InputOutput/FileReader.h"

namespace palo {

////////////////////////////////////////////////////////////////////////////////
/// @brief read a data file
///
/// The file reader reads data from a CSV file.
////////////////////////////////////////////////////////////////////////////////

class SERVER_CLASS FileReaderTXT : public FileReader {
public:

	////////////////////////////////////////////////////////////////////////////////
	/// @brief constructs a new file reader given a filename, path and file extension
	////////////////////////////////////////////////////////////////////////////////

	FileReaderTXT(const FileName& fileName);

	////////////////////////////////////////////////////////////////////////////////
	/// @brief deletes a file reader
	////////////////////////////////////////////////////////////////////////////////

	virtual ~FileReaderTXT();

public:

	////////////////////////////////////////////////////////////////////////////////
	/// @brief opens the file for reading
	////////////////////////////////////////////////////////////////////////////////

	virtual bool openFile(bool throwError, bool skipMessage);

public:

	////////////////////////////////////////////////////////////////////////////////
	/// @brief returns true if the actual line is a data line
	////////////////////////////////////////////////////////////////////////////////

	virtual bool isDataLine() const {
		return data;
	}
	;

	////////////////////////////////////////////////////////////////////////////////
	/// @brief returns true if the actual line is a section line
	////////////////////////////////////////////////////////////////////////////////

	virtual bool isSectionLine() const {
		return section;
	}
	;

	////////////////////////////////////////////////////////////////////////////////
	/// @brief returns true if end of file reached
	////////////////////////////////////////////////////////////////////////////////

	virtual bool isEndOfFile() const {
		return endOfFile;
	}
	;

	////////////////////////////////////////////////////////////////////////////////
	/// @brief returns the last section name
	////////////////////////////////////////////////////////////////////////////////

	virtual const string& getSection() const {
		return sectionName;
	}
	;

	////////////////////////////////////////////////////////////////////////////////
	/// @brief get number of entries
	////////////////////////////////////////////////////////////////////////////////

	virtual size_t getDataSize() const {
		return dataLine.size();
	}

	////////////////////////////////////////////////////////////////////////////////
	/// @brief get position num as string of actual line
	////////////////////////////////////////////////////////////////////////////////

	virtual const string& getDataString(int num) const;

	////////////////////////////////////////////////////////////////////////////////
	/// @brief get position num as integer of actual line
	////////////////////////////////////////////////////////////////////////////////

	virtual long getDataInteger(int num, int defaultValue = 0) const;

	////////////////////////////////////////////////////////////////////////////////
	/// @brief get position num as double of actual line
	////////////////////////////////////////////////////////////////////////////////

	virtual double getDataDouble(int num) const;

	////////////////////////////////////////////////////////////////////////////////
	/// @brief get position num as boolean of actual line
	////////////////////////////////////////////////////////////////////////////////

	virtual bool getDataBool(int num, bool defaultValue = false) const;

	////////////////////////////////////////////////////////////////////////////////
	/// @brief get position num as a vector of strings
	////////////////////////////////////////////////////////////////////////////////

	virtual const vector<string> getDataStrings(int num, char separator);

	////////////////////////////////////////////////////////////////////////////////
	/// @brief get position num as a vector of integers
	////////////////////////////////////////////////////////////////////////////////

	virtual const vector<int> getDataIntegers(int num);

	////////////////////////////////////////////////////////////////////////////////
	/// @brief get position num as a vector of identifiers
	////////////////////////////////////////////////////////////////////////////////

	virtual const IdentifiersType getDataIdentifiers(int num);

	////////////////////////////////////////////////////////////////////////////////
	/// @brief get position num as area - vector of vectors of identifiers
	////////////////////////////////////////////////////////////////////////////////

	virtual PArea getDataArea(int num, size_t dimCount);

	////////////////////////////////////////////////////////////////////////////////
	/// @brief get position num as paths - vector of vectors of identifiers
	////////////////////////////////////////////////////////////////////////////////

	virtual PPaths getDataPaths(int num);

	////////////////////////////////////////////////////////////////////////////////
	/// @brief get position num as a vector of doubles
	////////////////////////////////////////////////////////////////////////////////

	virtual const vector<double> getDataDoubles(int num);

	////////////////////////////////////////////////////////////////////////////////
	/// @brief get position num as a timestamp
	////////////////////////////////////////////////////////////////////////////////

	virtual void getTimeStamp(long *seconds, long *useconds, int num);

	////////////////////////////////////////////////////////////////////////////////
	/// @brief goto next line
	////////////////////////////////////////////////////////////////////////////////

	virtual void nextLine(bool strip = false);

	////////////////////////////////////////////////////////////////////////////////
	/// @brief get file line number
	////////////////////////////////////////////////////////////////////////////////

	virtual int32_t getLineNumber() {
		return lineNumber;
	}

	virtual void getRaw(char *p, streamsize size);

protected:

	////////////////////////////////////////////////////////////////////////////////
	/// @brief reads actual line and fills "sectionName"
	////////////////////////////////////////////////////////////////////////////////

	void processSection(const string& line);

	////////////////////////////////////////////////////////////////////////////////
	/// @brief splits a line into a vector and fills "dataLine"
	////////////////////////////////////////////////////////////////////////////////

	void processDataLine(string& line);

	////////////////////////////////////////////////////////////////////////////////
	/// @brief splits a string at a given seperator into a vector of strings
	////////////////////////////////////////////////////////////////////////////////

	void splitString(string& line, vector<string>* elements, char seperator, bool readNextLine = false);

protected:

	////////////////////////////////////////////////////////////////////////////////
	/// @brief true if the actual line is a data line
	////////////////////////////////////////////////////////////////////////////////

	bool data;

	////////////////////////////////////////////////////////////////////////////////
	/// @brief true if the actual line is a section line
	////////////////////////////////////////////////////////////////////////////////

	bool section;

	////////////////////////////////////////////////////////////////////////////////
	/// @brief true if the end of file is reached
	////////////////////////////////////////////////////////////////////////////////

	bool endOfFile;

	////////////////////////////////////////////////////////////////////////////////
	/// @brief last section name
	////////////////////////////////////////////////////////////////////////////////

	string sectionName;

	////////////////////////////////////////////////////////////////////////////////
	/// @brief elements of the last data line
	////////////////////////////////////////////////////////////////////////////////

	vector<string> dataLine;

	////////////////////////////////////////////////////////////////////////////////
	/// @brief pointer to the file
	////////////////////////////////////////////////////////////////////////////////

	std::istream *inputFile;

	////////////////////////////////////////////////////////////////////////////////
	/// @brief line number
	////////////////////////////////////////////////////////////////////////////////

	int32_t lineNumber;
};

}

#endif

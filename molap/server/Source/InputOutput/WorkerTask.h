////////////////////////////////////////////////////////////////////////////////
/// @brief worker task
///
/// @file
///
/// Copyright (C) 2006-2008 Jedox AG
///
/// This program is free software; you can redistribute it and/or modify it
/// under the terms of the GNU General Public License (Version 2) as published
/// by the Free Software Foundation at http://www.gnu.org/copyleft/gpl.html.
///
/// This program is distributed in the hope that it will be useful, but WITHOUT
/// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
/// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
/// more details.
///
/// You should have received a copy of the GNU General Public License along with
/// this program; if not, write to the Free Software Foundation, Inc., 59 Temple
/// Place, Suite 330, Boston, MA 02111-1307 USA
/// 
/// You may obtain a copy of the License at
///
/// <a href="http://www.jedox.com/license_palo.txt">
///   http://www.jedox.com/license_palo.txt
/// </a>
///
/// If you are developing and distributing open source applications under the
/// GPL License, then you are free to use Palo under the GPL License.  For OEMs,
/// ISVs, and VARs who distribute Palo with their products, and do not license
/// and distribute their source code under the GPL, Jedox provides a flexible
/// OEM Commercial License.
///
/// Developed by triagens GmbH, Koeln on behalf of Jedox AG. Intellectual
/// property rights has triagens GmbH, Koeln. Exclusive worldwide
/// exploitation right (commercial copyright) has Jedox AG, Freiburg.
///
/// @author Frank Celler, triagens GmbH, Cologne, Germany
/// @author Achim Brandt, triagens GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#ifndef INPUT_OUTPUT_WORKER_TASK_H
#define INPUT_OUTPUT_WORKER_TASK_H 1

#include "palo.h"

#if defined(_MSC_VER)
#include "InputOutput/HandleTask.h"
#else
#include "InputOutput/ReadWriteTask.h"
#endif

namespace palo {
  class Worker;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief worker task
  ////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)
  class SERVER_CLASS WorkerTask : virtual public HandleTask {
  public:
    WorkerTask (Worker*, HANDLE readHandle, HANDLE writeHandle);

#else
  class WorkerTask : virtual public ReadWriteTask {
  public:
    WorkerTask (Worker*, socket_t readSocket, socket_t writeSocket);

#endif

    ~WorkerTask ();

  public:
    enum WorkerTaskStatus {
      READING,
      WRITING,
      IDLE,
    };

  public:
    void execute (const string& line);

  protected:
#if defined(_MSC_VER)
    void handleHangup (HANDLE);
    bool handleRead (size_t available);
    size_t canHandleRead ();
#else
    void handleHangup (socket_t);
    bool handleRead ();
    bool canHandleRead ();
#endif

    void setWriteBuffer (StringBuffer * sb, bool ownBuffer = true);
    void completedWriteBuffer ();

  private:
    Worker * worker;
    WorkerTaskStatus status;
    size_t readPosition; 
    vector<string> result;
 };

}

#endif

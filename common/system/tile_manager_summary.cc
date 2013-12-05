#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include "log.h"
#include "simulator.h"
#include "config.h"
#include "transport.h"
#include "tile.h"
#include "tile_manager.h"

using namespace std;

// -- outputSummary
//
// Collect output summaries for all the tiles and send them to process
// zero. This process then formats the output to look pretty. Only
// process zero writes to the output stream passed in.

static void gatherSummaries(vector<string> &summaries)
{
   Config *cfg = Config::getSingleton();
   Transport::Node *global_node = Transport::getSingleton()->getGlobalNode();

   for (UInt32 p = 0; p < cfg->getProcessCount(); p++)
   {
      LOG_PRINT("Collect from process %d", p);

      const Config::TileList &tl = cfg->getApplicationTileListForProcess(p);

      // signal process to send
      if (p != 0)
         global_node->globalSend(p, &p, sizeof(p));

      // receive summary
      for (UInt32 t = 0; t < tl.size(); t++)
      {
         LOG_PRINT("Collect from tile %d", tl[t]);

         Byte *buf;

         buf = global_node->recv();
         assert(*((tile_id_t*)buf) == tl[t]);
         delete [] buf;

         buf = global_node->recv();
         summaries[tl[t]] = string((char*)buf);
         delete [] buf;
      }
   }

   for (UInt32 i = 0; i < summaries.size(); i++)
   {
      LOG_ASSERT_ERROR(!summaries[i].empty(), "Summary %d is empty!", i);
   }

   LOG_PRINT("Done collecting.");
}

class Table
{
public:
   Table(unsigned int rows,
         unsigned int cols)
      : m_table(rows * cols)
      , m_rows(rows)
      , m_cols(cols)
   { }

   string& operator () (unsigned int r, unsigned int c)
   {
      return at(r,c);
   }
   
   string& at(unsigned int r, unsigned int c)
   {
      assert(r < rows() && c < cols());
      return m_table[ r * m_cols + c ];
   }

   const string& at(unsigned int r, unsigned int c) const
   {
      assert(r < rows() && c < cols());
      return m_table[ r * m_cols + c ];
   }

   string flatten() const
   {
      vector<unsigned int> col_widths;

      for (unsigned int i = 0; i < cols(); i++)
         col_widths.push_back(0);

      for (unsigned int r = 0; r < rows(); r++)
         for (unsigned int c = 0; c < cols(); c++)
            if (at(r,c).length() > col_widths[c])
               col_widths[c] = at(r,c).length();

      stringstream out;

      for (unsigned int r = 0; r < rows(); r++)
      {
         for (unsigned int c = 0; c < cols(); c++)
         {
            out << at(r,c);
            
            unsigned int padding = col_widths[c] - at(r,c).length();
            out << string(padding, ' ') << " | ";
         }

         out << '\n';
      }

      return out.str();
   }

   unsigned int rows() const
   {
      return m_rows;
   }

   unsigned int cols() const
   {
      return m_cols;
   }

   typedef vector<string>::size_type size_type;

private:
   vector<string> m_table;
   unsigned int m_rows;
   unsigned int m_cols;
};

void addRowHeadings(Table &table, const vector<string> &summaries)
{
   // take row headings from first summary

   const string &sum = summaries[0];
   string::size_type pos = 0;

   for (Table::size_type i = 1; i < table.rows(); i++)
   {
      string::size_type end = sum.find(':', pos);
      string heading = sum.substr(pos, end-pos);
      pos = sum.find('\n', pos) + 1;
      assert(pos != string::npos);

      table(i,0) = heading;
   }
}

void addColHeadings(Table &table)
{
   for (Table::size_type i = 0; i < Config::getSingleton()->getApplicationTiles(); i++)
   {
      stringstream heading;
      heading << "Tile " << i;
      table(0, i+1) = heading.str();
   }
}

void addTileSummary(Table &table, tile_id_t tile, const string &summary)
{
   string::size_type pos = summary.find(':')+1;

   for (Table::size_type i = 1; i < table.rows(); i++)
   {
      string::size_type end = summary.find('\n',pos);
      string value = summary.substr(pos, end-pos);
      pos = summary.find(':',pos)+1;
      assert(pos != string::npos);

      table(i, tile+1) = value;
   }
}

string formatSummaries(const vector<string> &summaries)
{
   // assume that each tile outputs the same information
   // assume that output is formatted as "label: value"

   // from first summary, find number of rows needed
   unsigned int rows = count(summaries[0].begin(), summaries[0].end(), '\n');

   // fill in row headings
   Table table(rows+1, Config::getSingleton()->getApplicationTiles()+1);

   addRowHeadings(table, summaries);
   addColHeadings(table);

   for (unsigned int i = 0; i < summaries.size(); i++)
   {
      addTileSummary(table, i, summaries[i]);
   }

   return table.flatten();
}

void TileManager::outputSummary(ostream &os)
{
   LOG_PRINT("Starting TileManager::outputSummary");

   // Note: Using the global_node only works here because the lcp has
   // finished and therefore is no longer waiting on a receive. This
   // is not the most obvious thing, so maybe there should be a
   // cleaner solution.

   Config *cfg = Config::getSingleton();
   Transport::Node *global_node = Transport::getSingleton()->getGlobalNode();

   // wait for my turn...
   if (cfg->getCurrentProcessNum() != 0)
   {
      Byte *buf = global_node->recv();
      assert(*((UInt32*)buf) == cfg->getCurrentProcessNum());
      delete [] buf;
   }

   // send each summary
   const Config::TileList &tl = cfg->getApplicationTileListForProcess(cfg->getCurrentProcessNum());

   for (UInt32 i = 0; i < tl.size(); i++)
   {
      LOG_PRINT("Output summary tile %i", tl[i]);
      stringstream ss;
      m_tiles[i]->outputSummary(ss);
      global_node->globalSend(0, &tl[i], sizeof(tl[i]));
      global_node->globalSend(0, ss.str().c_str(), ss.str().length()+1);
   }

   // format (only done on proc 0)
   if (cfg->getCurrentProcessNum() != 0)
      return;

   vector<string> summaries(cfg->getApplicationTiles());
   string formatted;

   gatherSummaries(summaries);
   formatted = formatSummaries(summaries);

   os << formatted;                   

   LOG_PRINT("Finished outputSummary");
}

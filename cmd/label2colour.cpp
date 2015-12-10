/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2013.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/


#include <map>
#include <vector>

#include "command.h"
#include "image.h"
#include "mrtrix.h"

#include "algo/loop.h"
#include "math/rng.h"

#include "connectome/config/config.h"
#include "connectome/connectome.h"
#include "connectome/lut.h"



using namespace MR;
using namespace App;
using namespace MR::Connectome;


void usage ()
{

	AUTHOR = "Robert E. Smith (r.smith@brain.org.au)";

  DESCRIPTION
  + "convert a parcellated image (where values are node indices) into a colour image "
    "(many software packages handle this colouring internally within their viewer program; this binary "
    "explicitly converts a parcellation image into a colour image that should be viewable in any software)";

  ARGUMENTS
  + Argument ("nodes_in",   "the input node parcellation image").type_image_in()
  + Argument ("colour_out", "the output colour image").type_image_out();

  OPTIONS
  + LookupTableOption

  + Option ("config", "If the input parcellation image was created using labelconfig, provide the connectome config file used so that the node indices are converted correctly")
    + Argument ("file").type_file_in();

};





void run ()
{

  auto nodes = Image<node_t>::open (argument[0]);

  Node_map node_map;
  load_lut_from_cmdline (node_map);

  auto opt = get_options ("config");
  if (opt.size()) {

    if (node_map.empty())
      throw Exception ("Cannot properly interpret connectome config file if no lookup table is provided");

    ConfigInvLookup config;
    load_config (opt[0][0], config);

    // Now that we know the configuration, can convert the lookup table to reflect the new indices
    // If no corresponding entry exists in the config file, then the node doesn't get coloured
    Node_map new_node_map;
    for (Node_map::iterator i = node_map.begin(); i != node_map.end(); ++i) {
      ConfigInvLookup::const_iterator existing = config.find (i->second.get_name());
      if (existing != config.end())
        new_node_map.insert (std::make_pair (existing->second, i->second));
    }

    if (new_node_map.empty())
      throw Exception ("Config file and parcellation lookup table do not appear to belong to one another");
    new_node_map.insert (std::make_pair (0, Node_info ("Unknown", RGB (0, 0, 0), 0)));
    node_map = new_node_map;

  }


  if (node_map.empty()) {

    INFO ("No lookup table provided; colouring nodes randomly");

    node_t max_index = 0;
    for (auto l = Loop (nodes) (nodes); l; ++l) {
      const node_t index = nodes.value();
      if (index > max_index)
        max_index = index;
    }

    node_map.insert (std::make_pair (0, Node_info ("None", 0, 0, 0, 0)));
    Math::RNG rng;
    std::uniform_int_distribution<uint8_t> dist;

    for (node_t i = 1; i <= max_index; ++i) {
      RGB colour;
      do {
        colour[0] = dist (rng);
        colour[1] = dist (rng);
        colour[2] = dist (rng);
      } while (int(colour[0]) + int(colour[1]) + int(colour[2]) < 100);
      node_map.insert (std::make_pair (i, Node_info (str(i), colour)));
    }

  }


  Header H = nodes.original_header();
  H.set_ndim (4);
  H.size (3) = 3;
  H.datatype() = DataType::UInt8;
  add_line (H.keyval()["comments"], "Coloured parcellation image generated by label2colour");
  auto out = Image<uint8_t>::create (argument[1], H);

  for (auto l = Loop ("Colourizing parcellated node image", nodes) (nodes, out); l; ++l) {
    const node_t index = nodes.value();
    Node_map::const_iterator i = node_map.find (index);
    if (i == node_map.end()) {
      out.index (3) = 0; out.value() = 0;
      out.index (3) = 1; out.value() = 0;
      out.index (3) = 2; out.value() = 0;
    } else {
      const RGB& colour (i->second.get_colour());
      out.index (3) = 0; out.value() = colour[0];
      out.index (3) = 1; out.value() = colour[1];
      out.index (3) = 2; out.value() = colour[2];
    }
  }

}




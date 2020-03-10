/*******************************************************************************
 *
 * TRIQS: a Toolbox for Research in Interacting Quantum Systems
 *
 * Copyright (C) 2013, P. Seth, I. Krivenko, M. Ferrero and O. Parcollet
 *
 * TRIQS is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * TRIQS is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * TRIQS. If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/
#pragma once

#include <set>
#include <boost/container/flat_map.hpp>
#include <triqs/utility/exceptions.hpp>
#include <triqs/h5.hpp>
#include <triqs/h5/vector.hpp>
#include "fundamental_operator_set.hpp"

namespace triqs {
  namespace hilbert_space {

    /// The coding of the fermionic Fock state: 64 bits word in binary.
    using fock_state_t = uint64_t;

    /// A Hilbert space spanned from *all* fermionic Fock states generated by a given set of fundamental operators.
    /**
  @include triqs/hilbert_space/hilbert_space.hpp
 */
    class hilbert_space {
      int dim; // the dimension

      public:
      /// Construct a dummy Hilbert space of zero size
      hilbert_space() : dim(0) {}

      /// Construct from a given fundamental operator set
      /**
   @param fops Generating fundamental operator set
 */
      hilbert_space(fundamental_operator_set const &fops) : dim(1ull << fops.size()) {}

      /// Return the total number of the fermionic Fock states in this space
      /**
   @return Size of the Hilbert space
 */
      int size() const { return dim; }

      /// Check two Hilbert spaces for equality
      /**
   For the full Hilbert spaces this check is equivalent to a dimension equality check

   @param hs Another Hilbert space
   @return `true` if the two spaces are equal, `false` otherwise
 */
      bool operator==(hilbert_space const &hs) const { return dim == hs.dim; }

      /// Check two Hilbert spaces for inequality
      /**
   For the full Hilbert spaces this check is equivalent to a dimension inequality check
   @param hs Another Hilbert space
   @return `false` if the two spaces are equal, `true` otherwise
 */
      bool operator!=(hilbert_space const &hs) const { return !operator==(hs); }

      /// Check if a given Fock state belongs to this space
      /**
   @param f Fock state in question
   @return `true` if `f` belongs to the space, `false` otherwise
 */
      bool has_state(fock_state_t f) const { return f < dim; }

      /// Find the index of a given Fock state within this space
      /**
   @param f Fock state in question
   @return State index
 */
      int get_state_index(fock_state_t f) const {
        if (f >= dim) TRIQS_RUNTIME_ERROR << "This index is too big, f = " << f;
        return f;
      }

      /// Return the `i`-th basis element as a Fock state
      /**
   @param i Index of the basis state
   @return Fock state
 */
      fock_state_t get_fock_state(int i) const {
        if (i >= dim) TRIQS_RUNTIME_ERROR << "This Fock state does not exist (index too big), i = " << i;
        return i;
      }

      /// Return the Fock state generated by a product of creation operators with given indices
      /**
   @param fops Fundamental operator set used to construct this Hilbert space
   @param indices An ordered set of index sequences (see `indices_t` in [[fundamental_operator_set]])
   @return Fock state
 */
      fock_state_t get_fock_state(fundamental_operator_set const &fops, std::set<fundamental_operator_set::indices_t> const &indices) const {
        fock_state_t f = 0;
        for (auto const &index : indices) f += 1 << fops[index];
        return f;
      }

      /**
   @return Name of the scheme
 */
      static std::string hdf5_scheme() { return "hilbert_space"; }

      private:
      /// Write a Hilbert space to an HDF5 group
      /**
   @param fg Parent HDF5 group to write the space to
   @param name Name of the HDF5 subgroup to be created
   @param hs Hilbert space to be written
 */
      friend void h5_write(h5::group fg, std::string const &name, hilbert_space const &hs) {
        auto gr = fg.create_group(name);
        h5_write(gr, "dim", hs.dim);
      }

      /// Read a Hilbert space from an HDF5 group
      /**
   @param fg Parent HDF5 group to read the space from
   @param name Name of the HDF5 subgroup to be read
   @param hs Reference to a target Hilbert space object
 */
      friend void h5_read(h5::group fg, std::string const &name, hilbert_space &hs) {
        auto gr = fg.open_group(name);
        h5_read(gr, "dim", hs.dim);
      }
    };

    /// Hilbert subspace, as an ordered set of basis Fock states.
    /**
  Subspaces carry an integer index, which allows them to be destinguished as parts of a full Hilbert space.
  @include triqs/hilbert_space/hilbert_space.hpp
 */
    class sub_hilbert_space {

      public:
      /// Construct an empty Hilbert subspace
      /**
   @param index Index of this subspace within the full Hilbert space
 */
      sub_hilbert_space(int index = -1) : index(index) {}

#ifdef TRIQS_WORKAROUND_INTEL_COMPILER_BUGS
      // Workaround needed for icc, checked with 17.0.1 20161005)
      sub_hilbert_space(sub_hilbert_space const &) = default;
      sub_hilbert_space(sub_hilbert_space &&)      = default;
      sub_hilbert_space &operator                  =(sub_hilbert_space const &x) {
        index         = x.index;
        fock_states   = x.fock_states;
        fock_to_index = x.fock_to_index;
        return *this;
      }
      sub_hilbert_space &operator=(sub_hilbert_space &&) = default;
#endif

      /// Add a Fock state to the Hilbert space basis
      /**
   @param f Fock state to add
 */
      void add_fock_state(fock_state_t f) {
        int ind = fock_states.size();
        fock_states.push_back(f);
        fock_to_index.insert(std::make_pair(f, ind));
      }

      /// Return the total number of the fermionic Fock states in this space
      /**
   @return Size of the Hilbert subspace
 */
      int size() const { return fock_states.size(); }

      /// Check two Hilbert subspaces for equality
      /**
   Two subspaces are considered equal iff they have the same index and
   equal sets of basis Fock states.

   @param hs Another Hilbert subspace
   @return `true` if the two subspaces are equal, `false` otherwise
 */
      bool operator==(sub_hilbert_space const &hs) const { return index == hs.index && fock_states == hs.fock_states; }

      /// Check two Hilbert subspaces for inequality
      /**
   Two subspaces are considered equal iff they have the same index and
   equal sets of basis Fock states.

   @param hs Another Hilbert subspace
   @return `false` if the two subspaces are equal, `true` otherwise
 */
      bool operator!=(sub_hilbert_space const &hs) const { return !operator==(hs); }

      /// Find the index of a given Fock state within this subspace
      /**
   @param f Fock state in question
   @return State index
 */
      int get_state_index(fock_state_t f) const { return fock_to_index.find(f)->second; }

      /// Check if a given Fock state belongs to this subspace
      /**
   @param f Fock state in question
   @return `true` if `f` belongs to the subspace, `false` otherwise
 */
      bool has_state(fock_state_t f) const { return fock_to_index.count(f) == 1; }

      /// Return the `i`-th basis element as a Fock state
      /**
   @param i Index of the basis state
   @return Fock state
 */
      fock_state_t get_fock_state(int i) const { return fock_states[i]; }

      /// Return all basis Fock states in this subspace as `std::vector`
      /**
   @return Vector of all Fock states
 */
      std::vector<fock_state_t> const &get_all_fock_states() const { return fock_states; }

      /// Return the index of this subspace within the full Hilbert space
      /**
   @return Index of the subspace
 */
      int get_index() const { return index; };

      /// Set the index of this subspace within the full Hilbert space
      /**
   @param i Index of the subspace
 */
      void set_index(int i) { index = i; }

      private:
      // Index of this subspace, as a part of a full Hilbert space
      int index;

      // The list of all Fock states
      std::vector<fock_state_t> fock_states;

      // Reverse map to quickly find the index of a state.
      // The boost::container::flat_map is implemented as an ordered vector,
      // hence it is slow to insert (we don't care) but fast to look up (we do it a lot)
      boost::container::flat_map<fock_state_t, int> fock_to_index;

      public:
      /// Return name of the HDF5 scheme
      /**
   @return Name of the scheme
 */
      static std::string hdf5_scheme() { return "sub_hilbert_space"; }

      /// Write a Hilbert subspace to an HDF5 group
      /**
   @param fg Parent HDF5 group to write the space to
   @param name Name of the HDF5 subgroup to be created
   @param hs Hilbert subspace to be written
 */
      friend void h5_write(h5::group fg, std::string const &name, sub_hilbert_space const &hs) {
        auto gr = fg.create_group(name);
        h5_write(gr, "index", hs.index);
        h5_write(gr, "fock_states", hs.fock_states);
      }

      /// Read a Hilbert subspace from an HDF5 group
      /**
   @param fg Parent HDF5 group to read the space from
   @param name Name of the HDF5 subgroup to be read
   @param hs Reference to a target Hilbert subspace object
 */
      friend void h5_read(h5::group fg, std::string const &name, sub_hilbert_space &hs) {
        using h5::h5_read;
        auto gr = fg.open_group(name);
        h5_read(gr, "index", hs.index);
        h5_read(gr, "fock_states", hs.fock_states);
        hs.fock_to_index.clear();
        for (auto f : hs.fock_states) hs.fock_to_index.insert(std::make_pair(f, static_cast<int>(hs.fock_to_index.size())));
      }
    };
  } // namespace hilbert_space
} // namespace triqs

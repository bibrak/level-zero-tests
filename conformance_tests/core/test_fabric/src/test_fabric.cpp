/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include <unordered_map>

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

TEST(zeFabricVertexGetTests,
     GivenZeroCountWhenRetrievingFabricVerticesThenValidCountReturned) {
  auto vertex_count = lzt::get_ze_fabric_vertex_count();
  EXPECT_GT(vertex_count, 0);
}

TEST(
    zeFabricVertexGetTests,
    GivenValidCountWhenRetrievingFabricVerticesThenNotNullFabricVerticesAreReturned) {

  auto vertex_count = lzt::get_ze_fabric_vertex_count();
  ASSERT_GT(vertex_count, 0);
  auto vertices = lzt::get_ze_fabric_vertices(vertex_count);
  for (const auto &vertex : vertices) {
    EXPECT_NE(nullptr, vertex);
  }
}

TEST(
    zeFabricVertexGetProperties,
    GivenValidFabricVertexWhenRetrievingPropertiesThenValidPropertiesAreReturned) {

  auto vertices = lzt::get_ze_fabric_vertices();
  for (const auto &vertex : vertices) {
    ze_fabric_vertex_exp_properties_t properties =
        lzt::get_ze_fabric_vertex_properties(vertex);
    ze_device_handle_t device = nullptr;
    if (ZE_RESULT_SUCCESS == zeFabricVertexGetDeviceExp(vertex, &device)) {
      ze_device_properties_t device_properties{};
      device_properties = lzt::get_device_properties(device);
      EXPECT_EQ(0, memcmp(properties.uuid.id, device_properties.uuid.id,
                          sizeof(properties.uuid.id)));
      EXPECT_EQ(ZE_FABRIC_VERTEX_EXP_TYPE_DEVICE, properties.type);
      EXPECT_FALSE(properties.remote);

      ze_pci_ext_properties_t pciProperties{};
      if (ZE_RESULT_SUCCESS ==
          zeDevicePciGetPropertiesExt(device, &pciProperties)) {
        EXPECT_EQ(properties.address.bus, pciProperties.address.bus);
        EXPECT_EQ(properties.address.device, pciProperties.address.device);
        EXPECT_EQ(properties.address.function, pciProperties.address.function);
      }
    } else {
      EXPECT_TRUE(properties.remote);
    }
  }
}

TEST(
    zeFabricSubVertexGetTests,
    GivenValidCountWhenRetrievingFabricSubVerticesThenNotNullFabricVerticesAreReturned) {

  auto vertices = lzt::get_ze_fabric_vertices();
  for (auto &vertex : vertices) {
    ASSERT_NE(nullptr, vertex);

    auto count = lzt::get_ze_fabric_sub_vertices_count(vertex);

    ze_device_handle_t device = nullptr;
    if (ZE_RESULT_SUCCESS == zeFabricVertexGetDeviceExp(vertex, &device)) {
      EXPECT_EQ(lzt::get_ze_sub_device_count(device), count);
    }

    if (count > 0) {
      std::vector<ze_fabric_vertex_handle_t> sub_vertices(count);
      sub_vertices = lzt::get_ze_fabric_sub_vertices(vertex);
      for (const auto &sub_vertex : sub_vertices) {
        EXPECT_NE(nullptr, sub_vertex);
      }
    }
  }
}

TEST(
    zeFabricVertexGetProperties,
    GivenValidFabricSubVertexWhenRetrievingPropertiesThenValidPropertiesAreReturned) {

  auto vertices = lzt::get_ze_fabric_vertices();
  for (const auto &vertex : vertices) {
    std::vector<ze_fabric_vertex_handle_t> sub_vertices{};
    sub_vertices = lzt::get_ze_fabric_sub_vertices(vertex);
    for (const auto &sub_vertex : sub_vertices) {
      ze_fabric_vertex_exp_properties_t properties =
          lzt::get_ze_fabric_vertex_properties(sub_vertex);

      ze_device_handle_t device = nullptr;
      if (ZE_RESULT_SUCCESS ==
          zeFabricVertexGetDeviceExp(sub_vertex, &device)) {
        ze_device_properties_t device_properties{};
        device_properties = lzt::get_device_properties(device);
        EXPECT_EQ(0, memcmp(properties.uuid.id, device_properties.uuid.id,
                            sizeof(properties.uuid.id)));
        EXPECT_EQ(ZE_FABRIC_VERTEX_EXP_TYPE_SUBEVICE, properties.type);
        EXPECT_FALSE(properties.remote);

        ze_pci_ext_properties_t pciProperties{};
        if (ZE_RESULT_SUCCESS ==
            zeDevicePciGetPropertiesExt(device, &pciProperties)) {
          EXPECT_EQ(properties.address.bus, pciProperties.address.bus);
          EXPECT_EQ(properties.address.device, pciProperties.address.device);
          EXPECT_EQ(properties.address.function,
                    pciProperties.address.function);
        }
      } else {
        EXPECT_TRUE(properties.remote);
      }
    }
  }
}

TEST(zeDeviceGetFabricVertex,
     GivenValidDeviceAndSubDeviceWhenGettingVertexThenValidVertexIsReturned) {

  auto devices = lzt::get_ze_devices();
  for (const auto &device : devices) {

    std::vector<ze_device_handle_t> sub_devices{};
    sub_devices = lzt::get_ze_sub_devices(device);

    for (const auto &sub_device : sub_devices) {
      ze_fabric_vertex_handle_t vertex{};
      ze_device_handle_t device_vertex{};
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeDeviceGetFabricVertexExp(sub_device, &vertex));
      ASSERT_NE(vertex, nullptr);
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeFabricVertexGetDeviceExp(vertex, &device_vertex));
      ASSERT_NE(device_vertex, nullptr);
      EXPECT_EQ(device_vertex, sub_device);
    }

    ze_fabric_vertex_handle_t vertex{};
    ze_device_handle_t device_vertex{};
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeDeviceGetFabricVertexExp(device, &vertex));
    ASSERT_NE(vertex, nullptr);
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeFabricVertexGetDeviceExp(vertex, &device_vertex));
    ASSERT_NE(device_vertex, nullptr);
    EXPECT_EQ(device_vertex, device);
  }
}

// Fabric Edge related

static std::vector<ze_fabric_vertex_handle_t> fabric_get_all_vertices() {

  std::vector<ze_fabric_vertex_handle_t> all_vertices{};
  auto vertices = lzt::get_ze_fabric_vertices();
  for (const auto &vertex : vertices) {
    all_vertices.push_back(vertex);

    std::vector<ze_fabric_vertex_handle_t> sub_vertices{};
    sub_vertices = lzt::get_ze_fabric_sub_vertices(vertex);
    for (const auto &sub_vertex : sub_vertices) {
      all_vertices.push_back(sub_vertex);
    }
  }

  return all_vertices;
}

static std::vector<ze_fabric_edge_handle_t> fabric_get_all_edges() {

  std::vector<ze_fabric_edge_handle_t> all_edges{};
  std::vector<ze_fabric_vertex_handle_t> vertices = fabric_get_all_vertices();
  auto vertex_count = vertices.size();
  if (vertex_count >= 2) {
    for (auto &vertex_a : vertices) {
      for (auto &vertex_b : vertices) {
        auto edge_count = lzt::get_ze_fabric_edge_count(vertex_a, vertex_b);
        std::vector<ze_fabric_edge_handle_t> edges{};
        edges = lzt::get_ze_fabric_edges(vertex_a, vertex_b, edge_count);
        for (auto &edge : edges) {
          if (edge == nullptr) {
            continue;
          }

          if (std::count(all_edges.begin(), all_edges.end(), edge) == 0) {
            all_edges.push_back(edge);
          }
        }
      }
    }
  }

  return all_edges;
}

TEST(zeFabricEdgeGetTests,
     GivenZeroCountWhenRetrievingFabricEdgesThenValidCountReturned) {

  std::vector<ze_fabric_vertex_handle_t> vertices = fabric_get_all_vertices();
  auto vertex_count = vertices.size();
  if (vertex_count < 2) {
    LOG_WARNING << "Test not executed due to not enough vertices";
    return;
  }

  for (auto &vertex_a : vertices) {
    for (auto &vertex_b : vertices) {
      EXPECT_GE(lzt::get_ze_fabric_edge_count(vertex_a, vertex_b), 0);
    }
  }
}

TEST(zeFabricEdgeGetTests,
     GivenValidCountWhenRetrievingFabricEdgesThenValidFabricEdgesAreReturned) {

  std::vector<ze_fabric_vertex_handle_t> vertices = fabric_get_all_vertices();
  auto vertex_count = vertices.size();
  if (vertex_count < 2) {
    LOG_WARNING << "Test not executed due to not enough vertices";
    return;
  }

  for (auto &vertex_a : vertices) {
    for (auto &vertex_b : vertices) {
      auto edge_count = lzt::get_ze_fabric_edge_count(vertex_a, vertex_b);
      std::vector<ze_fabric_edge_handle_t> edges(edge_count);
      edges = lzt::get_ze_fabric_edges(vertex_a, vertex_b, edge_count);
      for (auto &edge : edges) {
        EXPECT_NE(edge, nullptr);
        ze_fabric_vertex_handle_t check_vertex_a = nullptr,
                                  check_vertex_b = nullptr;
        EXPECT_EQ(
            ZE_RESULT_SUCCESS,
            zeFabricEdgeGetVerticesExp(edge, &check_vertex_a, &check_vertex_b));
        EXPECT_TRUE(check_vertex_a == vertex_a || check_vertex_a == vertex_b);
        EXPECT_TRUE(check_vertex_b == vertex_a || check_vertex_b == vertex_b);
      }
    }
  }
}

TEST(zeFabricEdgeGetTests,
     GivenValidFabricEdgesThenValidEdgePropertiesAreReturned) {

  std::vector<ze_fabric_edge_handle_t> edges = fabric_get_all_edges();
  if (edges.size() == 0) {
    LOG_WARNING << "Test not executed due to not enough edges";
    return;
  }

  for (auto &edge : edges) {
    ze_fabric_edge_exp_properties_t property =
        lzt::get_ze_fabric_edge_properties(edge);
    EXPECT_NE(property.bandwidth, 0);
    EXPECT_TRUE(property.bandwidthUnit == ZE_BANDWIDTH_UNIT_BYTES_PER_NANOSEC ||
                property.bandwidthUnit == ZE_BANDWIDTH_UNIT_BYTES_PER_CLOCK);
    EXPECT_TRUE(property.latency == ZE_LATENCY_UNIT_NANOSEC ||
                property.latency == ZE_LATENCY_UNIT_CLOCK ||
                property.latency == ZE_LATENCY_UNIT_HOP);
    EXPECT_TRUE(property.duplexity ==
                    ZE_FABRIC_EDGE_EXP_DUPLEXITY_HALF_DUPLEX ||
                property.duplexity == ZE_FABRIC_EDGE_EXP_DUPLEXITY_FULL_DUPLEX);
  }
}

TEST(zeFabricEdgeGetTests, GivenValidFabricEdgesThenEdgePropertyUuidIsUnique) {

  std::vector<ze_fabric_edge_handle_t> edges = fabric_get_all_edges();
  if (edges.size() == 0) {
    LOG_WARNING << "Test not executed due to not enough edges";
    return;
  }

  std::vector<ze_uuid_t> uuids{};

  auto is_uuid_unique = [uuids](ze_uuid_t &uuid) {
    for (auto &prev_uuid : uuids) {
      int status = std::memcmp(prev_uuid.id, uuid.id, sizeof(prev_uuid.id));
      if (status == 0) {
        return false;
      }
    }
    return true;
  };

  for (auto &edge : edges) {
    ze_fabric_edge_exp_properties_t property =
        lzt::get_ze_fabric_edge_properties(edge);
    const bool unique_status = is_uuid_unique(property.uuid);
    EXPECT_TRUE(unique_status);
    if (unique_status == true) {
      uuids.push_back(property.uuid);
    } else {
      break;
    }
  }
}

static void fabric_vertex_copy_memory(ze_fabric_vertex_handle_t &vertex_a,
                                      ze_fabric_vertex_handle_t &vertex_b,
                                      uint32_t copy_size) {

  ze_device_handle_t device_a{}, device_b{};
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeFabricVertexGetDeviceExp(vertex_a, &device_a));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeFabricVertexGetDeviceExp(vertex_b, &device_b));
  ASSERT_TRUE(lzt::can_access_peer(device_a, device_b));
  LOG_DEBUG << "Copy memory from (vertex: " << vertex_a
            << " device: " << device_a
            << ")"
               " to (vertex: "
            << vertex_b << " device: " << device_b << ")" << std::endl;

  auto cmdlist_a = lzt::create_command_list(device_a);
  auto cmdqueue_a = lzt::create_command_queue(device_a);
  auto cmdlist_b = lzt::create_command_list(device_b);
  auto cmdqueue_b = lzt::create_command_queue(device_b);

  const size_t size = copy_size;
  auto memory_a = lzt::allocate_shared_memory(size, device_a);
  auto memory_b = lzt::allocate_shared_memory(size, device_b);
  uint8_t pattern_a = 0xAB;
  uint8_t pattern_b = 0xFF;
  const int pattern_size = 1;

  // Fill with default pattern for both devices
  lzt::append_memory_fill(cmdlist_a, memory_a, &pattern_a, pattern_size, size,
                          nullptr);
  lzt::append_barrier(cmdlist_a, nullptr, 0, nullptr);
  lzt::close_command_list(cmdlist_a);
  lzt::execute_command_lists(cmdqueue_a, 1, &cmdlist_a, nullptr);
  lzt::synchronize(cmdqueue_a, UINT64_MAX);
  lzt::reset_command_list(cmdlist_a);

  lzt::append_memory_fill(cmdlist_b, memory_b, &pattern_b, pattern_size, size,
                          nullptr);
  lzt::append_barrier(cmdlist_b, nullptr, 0, nullptr);
  lzt::close_command_list(cmdlist_b);
  lzt::execute_command_lists(cmdqueue_b, 1, &cmdlist_b, nullptr);
  lzt::synchronize(cmdqueue_b, UINT64_MAX);
  lzt::reset_command_list(cmdlist_b);

  // Do memory copy between devices
  lzt::append_memory_copy(cmdlist_a, memory_b, memory_a, size);
  lzt::append_barrier(cmdlist_a, nullptr, 0, nullptr);
  lzt::close_command_list(cmdlist_a);
  lzt::execute_command_lists(cmdqueue_a, 1, &cmdlist_a, nullptr);
  lzt::synchronize(cmdqueue_a, UINT64_MAX);

  for (uint32_t i = 0; i < size; i++) {
    EXPECT_EQ(static_cast<uint8_t *>(memory_b)[i], pattern_a)
        << "Memory Fill did not match.";
  }

  lzt::free_memory(memory_a);
  lzt::destroy_command_queue(cmdqueue_a);
  lzt::destroy_command_list(cmdlist_a);

  lzt::free_memory(memory_b);
  lzt::destroy_command_queue(cmdqueue_b);
  lzt::destroy_command_list(cmdlist_b);
}

class zeFabricEdgeCopyTests : public ::testing::Test,
                              public ::testing::WithParamInterface<uint32_t> {};

TEST_P(zeFabricEdgeCopyTests,
       GivenValidFabricEdgesThenCopyIsSuccessfulBetweenThem) {

  std::vector<ze_fabric_edge_handle_t> edges = fabric_get_all_edges();
  if (edges.size() == 0) {
    LOG_WARNING << "Test not executed due to not enough edges";
    return;
  }

  uint32_t copy_size = GetParam();
  LOG_DEBUG << "Test Copy Size " << copy_size;

  for (auto &edge : edges) {
    ze_fabric_vertex_handle_t vertex_a = nullptr, vertex_b = nullptr;
    ASSERT_EQ(ZE_RESULT_SUCCESS,
              zeFabricEdgeGetVerticesExp(edge, &vertex_a, &vertex_b));

    fabric_vertex_copy_memory(vertex_a, vertex_b, copy_size);
  }
}

INSTANTIATE_TEST_CASE_P(zeFabricEdgeCopyTestAlignedAllocations,
                        zeFabricEdgeCopyTests,
                        ::testing::Values(1024u * 1024u, 64u * 1024u,
                                          8u * 1024u, 1u * 1024u, 64u, 1u));

} // namespace

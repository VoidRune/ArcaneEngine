#pragma once
#include "Collision.h"

#include <array>
#include <iostream>

template<typename T>
class StaticQuadTree
{
public:
	StaticQuadTree() { };
	~StaticQuadTree() { };

	void Resize(Rectangle area, int32_t maxDepth = 8)
	{
		m_MaxDepth = maxDepth;

		int32_t nodeCount = 1;
		// nodeCount = 4^0 + 4^1 + 4^2 + 4^3 + ... + 4^(maxDepth - 1)
		for (size_t i = 1; i < maxDepth; i++)
		{
			int32_t p = 1;
			for (size_t j = 0; j < i; j++)
				p *= 4;
			nodeCount += p;
		}

		m_Nodes.resize(nodeCount);

		Fill(area, 0, 1);
	}

	void Insert(Rectangle r, T item)
	{
		int index = 0;
		for (size_t d = 1; d < m_MaxDepth; d++)
		{
			int c = index * 4;
			if (Inside(r, m_Nodes[c + 1].Area))
				index = c + 1;
			else if (Inside(r, m_Nodes[c + 2].Area))
				index = c + 2;
			else if (Inside(r, m_Nodes[c + 3].Area))
				index = c + 3;
			else if (Inside(r, m_Nodes[c + 4].Area))
				index = c + 4;
			else
			{
				m_Nodes[index].Items.push_back(item);
				return;
			}
		}
	}
private:

	void Fill(Rectangle r, int32_t index, int32_t depth)
	{
		//std::cout << index << ", " << depth << std::endl;
		m_Nodes[index].Area = r;
		if (depth < m_MaxDepth)
		{
			Rectangle TL{ r.Position + glm::vec2{-r.Width / 4.0f, r.Height / 4.0f}, r.Width / 2.0f, r.Height / 2.0f, 0.0f };
			Rectangle TR{ r.Position + glm::vec2{ r.Width / 4.0f, r.Height / 4.0f}, r.Width / 2.0f, r.Height / 2.0f, 0.0f };
			Rectangle BL{ r.Position + glm::vec2{ r.Width / 4.0f,-r.Height / 4.0f}, r.Width / 2.0f, r.Height / 2.0f, 0.0f };
			Rectangle BR{ r.Position + glm::vec2{-r.Width / 4.0f,-r.Height / 4.0f}, r.Width / 2.0f, r.Height / 2.0f, 0.0f };
			Fill(TL, index * 4 + 1, depth + 1);
			Fill(TR, index * 4 + 2, depth + 1);
			Fill(BL, index * 4 + 3, depth + 1);
			Fill(BR, index * 4 + 4, depth + 1);
		}
	}

	class Node
	{
	public:
		int32_t Depth;
		Rectangle Area;

		//std::array<int32_t, 4> Children;
		std::vector<T> Items;
	};

	int32_t m_MaxDepth = 1;
	// Child indices of Node index X are X * 4 + (1/2/3/4)
	// Node at index 0 has children at indices 1, 2, 3, 4
	// Node at index 3 has children at indices 13, 14, 15, 16
	std::vector<Node> m_Nodes;
};